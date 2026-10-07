import React from 'react';

export default function CityMap({ nodes, roads, startNode, destNode, routeNodeIds, vehicles, onNodeClick }) {
  // Deduplicate roads visually
  const visualRoads = [];
  const seenRoads = new Set();
  
  roads.forEach(r => {
    const min = Math.min(r.from, r.to);
    const max = Math.max(r.from, r.to);
    const key = `${min}-${max}`;
    if (!seenRoads.has(key)) {
      seenRoads.add(key);
      visualRoads.push({...r});
    } else {
      const existing = visualRoads.find(vr => Math.min(vr.from, vr.to) === min && Math.max(vr.from, vr.to) === max);
      if (existing) {
        if (r.closed) existing.closed = true;
        if (r.trafficFactor > existing.trafficFactor) existing.trafficFactor = r.trafficFactor;
      }
    }
  });

  const routeSegments = new Set();
  if (routeNodeIds && routeNodeIds.length > 1) {
    for (let i = 0; i < routeNodeIds.length - 1; i++) {
      const min = Math.min(routeNodeIds[i], routeNodeIds[i + 1]);
      const max = Math.max(routeNodeIds[i], routeNodeIds[i + 1]);
      routeSegments.add(`${min}-${max}`);
    }
  }

  const nodeMap = new Map();
  nodes.forEach(n => nodeMap.set(n.id, n));

  const getTrafficColor = (r) => {
    if (r.closed) return 'var(--danger)'; 
    if (r.trafficFactor >= 5.0) return 'var(--danger)';
    if (r.trafficFactor >= 2.0) return 'var(--warning)';
    if (r.trafficFactor > 1.0) return '#a371f7';
    return 'var(--border)';
  };

  return (
    <div style={{ width: '100%', height: '100%', position: 'relative' }}>
      <svg 
        viewBox="-50 -50 500 500" 
        style={{ width: '100%', height: '100%', backgroundColor: '#000' }}
      >
        {/* Layer 1: Base roads & Traffic/Incidents */}
        {visualRoads.map(r => {
          const min = Math.min(r.from, r.to);
          const max = Math.max(r.from, r.to);
          const key = `${min}-${max}`;
          
          const n1 = nodeMap.get(r.from);
          const n2 = nodeMap.get(r.to);
          if (!n1 || !n2) return null;

          const baseColor = getTrafficColor(r);
          const dashArray = r.closed ? "6,6" : "none";

          return (
            <line
              key={`base-${key}`}
              x1={n1.x} y1={n1.y} x2={n2.x} y2={n2.y}
              stroke={baseColor}
              strokeWidth={8}
              strokeLinecap="round"
              strokeDasharray={dashArray}
            />
          );
        })}

        {/* Layer 2: Active Route overlay */}
        {visualRoads.map(r => {
          const min = Math.min(r.from, r.to);
          const max = Math.max(r.from, r.to);
          const key = `${min}-${max}`;
          
          if (!routeSegments.has(key)) return null;

          const n1 = nodeMap.get(r.from);
          const n2 = nodeMap.get(r.to);
          if (!n1 || !n2) return null;

          return (
            <line
              key={`route-${key}`}
              x1={n1.x} y1={n1.y} x2={n2.x} y2={n2.y}
              stroke="var(--accent)"
              strokeWidth={14}
              strokeLinecap="round"
              opacity={0.8}
            />
          );
        })}

        {/* Layer 3: Nodes */}
        {nodes.map(n => {
          let fill = 'var(--text-muted)';
          let r = 5;
          let stroke = 'none';
          
          if (n.id === startNode) { 
            fill = 'var(--success)'; 
            r = 12;
            stroke = '#fff';
          }
          else if (n.id === destNode) { 
            fill = 'var(--danger)'; 
            r = 12;
            stroke = '#fff';
          }

          return (
            <circle
              key={n.id}
              cx={n.x} cy={n.y} r={r}
              fill={fill}
              stroke={stroke}
              strokeWidth={2}
              onClick={() => onNodeClick && onNodeClick(n.id)}
              style={{ cursor: 'pointer' }}
            >
              <title>Node {n.id}</title>
            </circle>
          );
        })}

        {/* Layer 4: Vehicles */}
        {vehicles.map(v => (
          <circle
            key={`v-${v.id}`}
            cx={v.x} cy={v.y} r={6}
            fill="#fff"
            stroke="var(--accent)"
            strokeWidth={2}
          >
            <title>Vehicle {v.id}</title>
          </circle>
        ))}
      </svg>
    </div>
  );
}
