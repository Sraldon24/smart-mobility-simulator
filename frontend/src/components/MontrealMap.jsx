import { useEffect, useMemo, useRef, useState } from "react";
import { Map, NavigationControl, setWorkerUrl } from "maplibre-gl";
import "maplibre-gl/dist/maplibre-gl.css";
import workerUrl from "maplibre-gl/dist/maplibre-gl-worker.mjs?url";

setWorkerUrl(workerUrl);
import {
  addMapLayers,
  collection,
  line,
  mapStyle,
  point,
  roadFeatures,
} from "../features/simulator/mapData";

export default function MontrealMap({
  nodes,
  roads,
  startNode,
  destNode,
  routeNodeIds,
  vehicles,
  onNodeClick,
}) {
  const container = useRef(null),
    map = useRef(null),
    latest = useRef({ nodes, onNodeClick });
  const [ready, setReady] = useState(false),
    [mapError, setMapError] = useState("");
  const nodeIndex = useMemo(
    () => new globalThis.Map(nodes.map((node) => [node.id, node])),
    [nodes],
  );
  const roadData = useMemo(
    () => roadFeatures(roads, nodeIndex),
    [roads, nodeIndex],
  );
  useEffect(() => {
    latest.current = { nodes, onNodeClick };
  }, [nodes, onNodeClick]);

  useEffect(() => {
    let instance;
    try {
      instance = new Map({
        container: container.current,
        style: mapStyle,
        center: [-73.5673, 45.505],
        zoom: 13,
      });
      map.current = instance;
      instance.addControl(
        new NavigationControl({ showCompass: false }),
        "top-right",
      );
      instance.on("load", () => {
        addMapLayers(instance);
        setReady(true);
      });
      instance.on("error", (event) => {
        console.error("Map rendering error:", event.error);
        setMapError(
          "Background map unavailable. The street network and route remain interactive.",
        );
      });
      instance.on("click", (event) => {
        const nearest = latest.current.nodes.reduce(
          (best, node) => {
            const screen = instance.project([node.x, node.y]);
            const distance = Math.hypot(
              screen.x - event.point.x,
              screen.y - event.point.y,
            );
            return distance < best.distance ? { id: node.id, distance } : best;
          },
          { id: null, distance: 45 },
        );
        if (nearest.id !== null) latest.current.onNodeClick(nearest.id);
      });
    } catch (error) {
      console.error("Map initialization failed:", error);
      queueMicrotask(() =>
        setMapError(
          "This browser could not start the map. Enable graphics acceleration or try another browser.",
        ),
      );
    }
    const observer = new ResizeObserver(() => instance?.resize());
    observer.observe(container.current);
    return () => {
      observer.disconnect();
      instance?.remove();
      map.current = null;
    };
  }, []);

  useEffect(() => {
    if (!ready || !nodes.length) return;
    // Complete OSM ways extend beyond the extract; keep the initial view downtown.
    map.current.fitBounds(
      [
        [-73.59, 45.485],
        [-73.55, 45.525],
      ],
      {
        padding: 35,
        duration: 500,
        maxZoom: 15,
      },
    );
  }, [ready, nodes]);
  useEffect(() => {
    if (ready) map.current.getSource("roads").setData(roadData);
  }, [ready, roadData]);
  useEffect(() => {
    if (!ready) return;
    const coordinates = routeNodeIds.flatMap((id) =>
      nodeIndex.has(id) ? [[nodeIndex.get(id).x, nodeIndex.get(id).y]] : [],
    );
    map.current
      .getSource("route")
      .setData(collection(coordinates.length > 1 ? [line(coordinates)] : []));
  }, [ready, routeNodeIds, nodeIndex]);
  useEffect(() => {
    if (!ready) return;
    map.current.getSource("endpoints").setData(
      collection(
        [
          [startNode, "#087f68"],
          [destNode, "#e17c50"],
        ].flatMap(([id, color]) => {
          const node = nodeIndex.get(id);
          return node ? [point([node.x, node.y], { color })] : [];
        }),
      ),
    );
  }, [ready, startNode, destNode, nodeIndex]);
  useEffect(() => {
    if (ready)
      map.current
        .getSource("vehicles")
        .setData(
          collection(
            vehicles.map((vehicle) =>
              point([vehicle.lon ?? vehicle.x, vehicle.lat ?? vehicle.y]),
            ),
          ),
        );
  }, [ready, vehicles]);

  return (
    <div className="montreal-map">
      <div ref={container} className="map-canvas" />
      {mapError && (
        <div className="map-error" role="alert">
          {mapError}
        </div>
      )}
    </div>
  );
}
