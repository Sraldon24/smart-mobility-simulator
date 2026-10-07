export const collection = (features) => ({
  type: "FeatureCollection",
  features,
});
export const point = (coordinates, properties = {}) => ({
  type: "Feature",
  geometry: { type: "Point", coordinates },
  properties,
});
export const line = (coordinates, properties = {}) => ({
  type: "Feature",
  geometry: { type: "LineString", coordinates },
  properties,
});

export function roadFeatures(roads, nodes) {
  const pairs = new Map();
  for (const road of roads) {
    const key = [road.from, road.to].sort((a, b) => a - b).join("-");
    const existing = pairs.get(key);
    if (!existing || road.closed || road.trafficFactor > existing.trafficFactor)
      pairs.set(key, road);
  }
  return collection(
    [...pairs.values()].flatMap((road) => {
      const from = nodes.get(road.from),
        to = nodes.get(road.to);
      return from && to
        ? [
            line(
              [
                [from.x, from.y],
                [to.x, to.y],
              ],
              {
                closed: road.closed,
                color: road.closed
                  ? "#c64d48"
                  : road.trafficFactor >= 2
                    ? "#d89836"
                    : "#99ada6",
              },
            ),
          ]
        : [];
    }),
  );
}

export const mapStyle = {
  version: 8,
  sources: {
    basemap: {
      type: "raster",
      tiles: ["https://tile.openstreetmap.org/{z}/{x}/{y}.png"],
      tileSize: 256,
      attribution:
        '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    },
  },
  layers: [
    {
      id: "basemap",
      type: "raster",
      source: "basemap",
      paint: { "raster-saturation": -0.6 },
    },
  ],
};

export function addMapLayers(map) {
  for (const name of ["roads", "route", "endpoints", "vehicles"])
    map.addSource(name, { type: "geojson", data: collection([]) });
  map.addLayer({
    id: "roads",
    type: "line",
    source: "roads",
    filter: ["!=", ["get", "closed"], true],
    paint: {
      "line-color": ["get", "color"],
      "line-width": ["interpolate", ["linear"], ["zoom"], 11, 0.6, 16, 2.5],
    },
  });
  map.addLayer({
    id: "closed-roads",
    type: "line",
    source: "roads",
    filter: ["==", ["get", "closed"], true],
    paint: {
      "line-color": "#c64d48",
      "line-width": 4,
      "line-dasharray": [2, 2],
    },
  });
  map.addLayer({
    id: "route",
    type: "line",
    source: "route",
    layout: { "line-join": "round", "line-cap": "round" },
    paint: { "line-color": "#087f68", "line-width": 6 },
  });
  map.addLayer({
    id: "endpoints",
    type: "circle",
    source: "endpoints",
    paint: {
      "circle-radius": 9,
      "circle-color": ["get", "color"],
      "circle-stroke-width": 3,
      "circle-stroke-color": "#fff",
    },
  });
  map.addLayer({
    id: "vehicles",
    type: "circle",
    source: "vehicles",
    paint: {
      "circle-radius": 4,
      "circle-color": "#356dbe",
      "circle-stroke-width": 1,
      "circle-stroke-color": "#fff",
    },
  });
}
