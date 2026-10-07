"""Runtime checks: python3 tests/test_deployment.py [backend executable]."""

import json
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(
    sys.argv.pop(1)
    if len(sys.argv) > 1 and not sys.argv[1].startswith("-")
    else ROOT / "build/backend/smart_mobility_backend"
).resolve()


class DeploymentTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        self.url = f"http://127.0.0.1:{port}"
        self.pbf = Path(self.directory.name) / "city.osm.pbf"
        self.log = tempfile.TemporaryFile()
        self.addCleanup(self.log.close)
        self.process = subprocess.Popen(
            [str(BINARY)],
            cwd=self.directory.name,
            env={**os.environ, "PORT": str(port), "OSM_PBF_PATH": str(self.pbf)},
            stdout=self.log,
            stderr=self.log,
        )
        self.addCleanup(self.stop_server)
        for _ in range(100):
            try:
                self.request("/health")
                return
            except URLError:
                time.sleep(0.05)
        self.fail("Backend failed to become healthy")

    def stop_server(self):
        self.process.terminate()
        try:
            self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()

    def request(self, path, body=None):
        data = json.dumps(body).encode() if body is not None else None
        request = Request(
            self.url + path, data=data, headers={"Content-Type": "application/json"}
        )
        with urlopen(request, timeout=30) as response:
            return json.load(response)

    def test_missing_montreal_preserves_generated_city(self):
        before = self.request("/city")
        self.assertEqual(len(before["nodes"]), 25)
        with self.assertRaises(HTTPError) as error:
            self.request("/mode", {"mode": "montreal"})
        self.assertEqual(error.exception.code, 503)
        error.exception.close()
        self.assertEqual(self.request("/city"), before)
        self.assertTrue(self.request("/route?start=0&end=24")["found"])
        self.assertEqual(self.request("/health")["status"], "ok")

    def test_fresh_montreal_import_is_geographic_and_switches_back(self):
        shutil.copyfile(ROOT / "data/montreal/downtown.osm.pbf", self.pbf)
        self.request("/mode", {"mode": "montreal"})
        city = self.request("/city")
        self.assertEqual(city["coordinateSystem"], "geographic")
        self.assertGreater(len(city["nodes"]), 100)
        self.assertGreater(len(city["roads"]), 100)
        self.assertTrue(
            all(-74 < n["lon"] < -73 and 45 < n["lat"] < 46 for n in city["nodes"])
        )
        road = city["roads"][0]
        route = self.request(f"/route?start={road['from']}&end={road['to']}")
        self.assertTrue(route["found"])
        self.request("/mode", {"mode": "generated"})
        self.assertEqual(len(self.request("/city")["nodes"]), 25)

    def test_switching_with_active_traffic_and_polling(self):
        shutil.copyfile(ROOT / "data/montreal/downtown.osm.pbf", self.pbf)
        with ThreadPoolExecutor(max_workers=4) as pool:
            for mode in ["montreal", "generated"] * 3:
                self.request("/simulation/vehicles/batch", {"count": 10})
                self.request("/simulation/step", {"deltaTimeSeconds": 1})
                requests = [pool.submit(self.request, "/mode", {"mode": mode})]
                requests += [
                    pool.submit(self.request, path)
                    for path in ["/city", "/vehicles", "/simulation"]
                ]
                for result in requests:
                    result.result()
                self.assertEqual(self.request("/simulation")["vehicleCount"], 0)
                self.assertEqual(self.request("/health")["status"], "ok")


if __name__ == "__main__":
    unittest.main()
