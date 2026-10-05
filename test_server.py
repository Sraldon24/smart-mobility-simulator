import subprocess
import time
import urllib.request
import json

p = subprocess.Popen(["./smart_mobility_backend"], cwd="backend/build")
time.sleep(2)

def post(url, data):
    req = urllib.request.Request(url, data=json.dumps(data).encode('utf-8'), method='POST')
    req.add_header('Content-Type', 'application/json')
    return urllib.request.urlopen(req).read()

def get(url):
    req = urllib.request.Request(url, method='GET')
    return urllib.request.urlopen(req).read()

try:
    print("Spawning 100 vehicles...")
    post("http://127.0.0.1:8400/simulation/vehicles/batch", {"count": 100})
    
    print("\nState before step:")
    print(json.loads(get("http://127.0.0.1:8400/simulation")))

    print("\nStepping 10 seconds...")
    post("http://127.0.0.1:8400/simulation/step", {"deltaTimeSeconds": 10.0})

    print("\nState after 10 seconds:")
    state = json.loads(get("http://127.0.0.1:8400/simulation"))
    print(json.dumps(state, indent=2))

    print("\nStepping 20 more seconds...")
    post("http://127.0.0.1:8400/simulation/step", {"deltaTimeSeconds": 20.0})
    state = json.loads(get("http://127.0.0.1:8400/simulation"))
    print(json.dumps(state, indent=2))

except Exception as e:
    print("FAILED", e)

p.kill()
