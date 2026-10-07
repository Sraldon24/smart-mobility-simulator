import React, { useEffect, useRef } from 'react';
import * as maplibregl from 'maplibre-gl';
import 'maplibre-gl/dist/maplibre-gl.css';

export default function MontrealMap({ nodes, roads, startNode, destNode, routeNodeIds, vehicles, incidents, onNodeClick }) {
  const mapContainer = useRef(null);
  const map = useRef(null);
  const nodeMapRef = useRef(new Map());

  // 1. Initialize Map
  useEffect(() => {
    if (map.current) return;
    map.current = new maplibregl.Map({
      container: mapContainer.current,
      style: {
        version: 8,
        sources: {
          'osm-tiles': {
            type: 'raster',
            tiles: [
              'https://basemaps.cartocdn.com/dark_all/{z}/{x}/{y}.png'
            ],
            tileSize: 256,
            attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors &copy; <a href="https://carto.com/attributions">CARTO</a>'
          }
        },
        layers: [
          {
            id: 'osm-tiles-layer',
            type: 'raster',
            source: 'osm-tiles',
            minzoom: 0,
            maxzoom: 19
          }
        ]
      },
      center: [-73.5673, 45.5017], // Montreal approximate
      zoom: 12
    });

    map.current.on('load', () => {
      // Sources
      map.current.addSource('roads', { type: 'geojson', data: { type: 'FeatureCollection', features: [] } });
      map.current.addSource('route', { type: 'geojson', data: { type: 'FeatureCollection', features: [] } });
      map.current.addSource('endpoints', { type: 'geojson', data: { type: 'FeatureCollection', features: [] } });
      map.current.addSource('vehicles', { type: 'geojson', data: { type: 'FeatureCollection', features: [] } });

      // Layers
      map.current.addLayer({
        id: 'roads-layer',
        type: 'line',
        source: 'roads',
        layout: {
          'line-join': 'round',
          'line-cap': 'round'
        },
        paint: {
          'line-color': ['get', 'color'],
          'line-width': 4,
          'line-dasharray': [
            'case',
            ['==', ['get', 'closed'], true],
            ['literal', [2, 2]],
            ['literal', [1]]
          ]
        }
      });

      map.current.addLayer({
        id: 'route-layer',
        type: 'line',
        source: 'route',
        layout: {
          'line-join': 'round',
          'line-cap': 'round'
        },
        paint: {
          'line-color': '#58a6ff',
          'line-width': 6
        }
      });

      map.current.addLayer({
        id: 'endpoints-layer',
        type: 'circle',
        source: 'endpoints',
        paint: {
          'circle-radius': ['get', 'radius'],
          'circle-color': ['get', 'color'],
          'circle-stroke-width': 2,
          'circle-stroke-color': '#0d1117'
        }
      });

      map.current.addLayer({
        id: 'vehicles-layer',
        type: 'circle',
        source: 'vehicles',
        paint: {
          'circle-radius': 5,
          'circle-color': '#58a6ff',
          'circle-stroke-width': 1,
          'circle-stroke-color': '#3182ce'
        }
      });

      // Click event for setting nodes
      map.current.on('click', (e) => {
        // Find nearest node manually or query rendered features
        // Since we don't render all nodes as clickable objects (too many), we calculate distance.
        if (nodeMapRef.current.size === 0) return;
        const pt = e.lngLat;
        let closestId = null;
        let minDist = Infinity;
        
        for (const [id, n] of nodeMapRef.current.entries()) {
          const dx = n.lon - pt.lng;
          const dy = n.lat - pt.lat;
          const dist = dx*dx + dy*dy;
          if (dist < minDist) {
            minDist = dist;
            closestId = id;
          }
        }
        
        // If it's reasonably close, select it
        if (closestId !== null && minDist < 0.001) { // rough distance
          onNodeClick(closestId);
        }
      });
      
      // Initial trigger if data already loaded before map style finished
      updateRoads();
    });
  }, []);

  const updateRoads = () => {
    if (!map.current || !map.current.isStyleLoaded()) return;
    
    // Deduplicate and style roads
    const features = [];
    const seen = new Set();
    
    const getTrafficColor = (r) => {
      if (r.closed) return '#da3633'; // Closed
      if (r.trafficFactor >= 5.0) return '#da3633'; // Accident/Very congested
      if (r.trafficFactor >= 2.0) return '#d29922'; // Congested
      if (r.trafficFactor > 1.0) return '#a371f7'; // Moderate
      return '#30363d'; // Free
    };

    roads.forEach(r => {
      const min = Math.min(r.from, r.to);
      const max = Math.max(r.from, r.to);
      const key = `${min}-${max}`;
      
      if (!seen.has(key)) {
        seen.add(key);
        const n1 = nodeMapRef.current.get(r.from);
        const n2 = nodeMapRef.current.get(r.to);
        if (n1 && n2) {
          features.push({
            type: 'Feature',
            geometry: {
              type: 'LineString',
              coordinates: [[n1.lon, n1.lat], [n2.lon, n2.lat]]
            },
            properties: {
              color: getTrafficColor(r),
              closed: r.closed || false
            }
          });
        }
      }
    });

    map.current.getSource('roads').setData({ type: 'FeatureCollection', features });
  };

  // 2. Base Nodes and Bounds
  useEffect(() => {
    const nodeMap = new Map();
    let minLng = Infinity, minLat = Infinity, maxLng = -Infinity, maxLat = -Infinity;
    nodes.forEach(n => {
      nodeMap.set(n.id, n);
      if (n.lon < minLng) minLng = n.lon;
      if (n.lat < minLat) minLat = n.lat;
      if (n.lon > maxLng) maxLng = n.lon;
      if (n.lat > maxLat) maxLat = n.lat;
    });
    nodeMapRef.current = nodeMap;

    if (nodes.length > 0 && map.current) {
      if (minLng !== Infinity) {
        map.current.fitBounds([[minLng, minLat], [maxLng, maxLat]], { padding: 20 });
      }
      updateRoads();
    }
  }, [nodes]);

  // 3. Roads / Traffic updates
  useEffect(() => {
    updateRoads();
  }, [roads]);

  // 4. Route
  useEffect(() => {
    if (!map.current || !map.current.isStyleLoaded()) return;
    
    let coords = [];
    if (routeNodeIds && routeNodeIds.length > 0) {
      coords = routeNodeIds.map(id => {
        const n = nodeMapRef.current.get(id);
        return n ? [n.lon, n.lat] : null;
      }).filter(c => c !== null);
    }
    
    const features = [];
    if (coords.length > 1) {
      features.push({
        type: 'Feature',
        geometry: {
          type: 'LineString',
          coordinates: coords
        }
      });
      // Optionally fit bounds to route
      const bounds = coords.reduce((bounds, coord) => {
        return bounds.extend(coord);
      }, new maplibregl.LngLatBounds(coords[0], coords[0]));
      map.current.fitBounds(bounds, { padding: 50, maxZoom: 15 });
    }
    
    map.current.getSource('route').setData({ type: 'FeatureCollection', features });
  }, [routeNodeIds]);

  // 5. Start / Dest Endpoints
  useEffect(() => {
    if (!map.current || !map.current.isStyleLoaded()) return;
    
    const features = [];
    const addNode = (id, color, radius) => {
      const n = nodeMapRef.current.get(id);
      if (n) {
        features.push({
          type: 'Feature',
          geometry: { type: 'Point', coordinates: [n.lon, n.lat] },
          properties: { color, radius }
        });
      }
    };
    
    if (startNode !== null) addNode(startNode, '#238636', 8);
    if (destNode !== null) addNode(destNode, '#da3633', 8);
    
    map.current.getSource('endpoints').setData({ type: 'FeatureCollection', features });
  }, [startNode, destNode]);

  // 6. Vehicles
  useEffect(() => {
    if (!map.current || !map.current.isStyleLoaded()) return;
    const features = vehicles.map(v => ({
      type: 'Feature',
      geometry: { type: 'Point', coordinates: [v.lon !== undefined ? v.lon : v.x, v.lat !== undefined ? v.lat : v.y] },
      properties: { id: v.id }
    }));
    map.current.getSource('vehicles').setData({ type: 'FeatureCollection', features });
  }, [vehicles]);

  return (
    <div style={{ position: 'relative', width: '100%', height: '100%', overflow: 'hidden' }}>
      <div ref={mapContainer} style={{ width: '100%', height: '100%' }} />
    </div>
  );
}
