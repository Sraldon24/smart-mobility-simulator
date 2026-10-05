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

  // Identify route segments
  const routeSegments = new Set();
  if (routeNodeIds && routeNodeIds.length > 1) {
    for (let i = 0; i < routeNodeIds.length - 1; i++) {
      const min = Math.min(routeNodeIds[i], routeNodeIds[i + 1]);
      const max = Math.max(routeNodeIds[i], routeNodeIds[i + 1]);
      routeSegments.add(`${min}-${max}`);
    }
  }

  // Create lookup for node coordinates
  const nodeMap = new Map();
  nodes.forEach(n => nodeMap.set(n.id, n));

  const getTrafficColor = (r) => {
    if (r.closed) return '#8e44ad'; // Closed (purple)
    if (r.trafficFactor >= 5.0) return '#c0392b'; // Accident/Very congested
    if (r.trafficFactor >= 2.0) return '#e67e22'; // Congested
    if (r.trafficFactor > 1.0) return '#f1c40f'; // Moderate
    return '#bdc3c7'; // Free
  };

  return (
    <div style={{ width: '100%', position: 'relative' }}>
      <svg 
        viewBox="-50 -50 500 500" 
        style={{ width: '100%', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6' }}
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
          
          return (
            <line
              key={`route-${key}`}
              x1={n1.x} y1={n1.y} x2={n2.x} y2={n2.y}
              stroke="#3498db"
              strokeWidth={4}
              strokeLinecap="round"
            />
          );
        })}

        {/* Layer 3: Nodes */}
        {nodes.map(n => {
          const isStart = n.id === startNode;
          const isDest = n.id === destNode;
          
          let fill = '#95a5a6';
          let stroke = '#7f8c8d';
          let r = 12;

          if (isStart) {
            fill = '#2ecc71';
            stroke = '#27ae60';
            r = 16;
          } else if (isDest) {
            fill = '#e74c3c';
            stroke = '#c0392b';
            r = 16;
          }

          return (
            <g 
              key={n.id} 
              transform={`translate(${n.x},${n.y})`} 
              onClick={() => onNodeClick(n.id)}
              style={{ cursor: 'pointer' }}
            >
               <circle r={r} fill={fill} stroke={stroke} strokeWidth={3} />
              <text 
                textAnchor="middle" dy=".3em" fill="#ffffff" 
                fontSize={isStart || isDest ? "14" : "12"}
                fontWeight="bold" pointerEvents="none"
              >
                {n.id}
              </text>
            </g>
          );
        })}
      {/* Layer 4: Vehicles */}
        {vehicles && vehicles.map(v => (
          <g key={`vehicle-${v.id}`} transform={`translate(${v.x},${v.y})`}>
            <circle r="5" fill="#e74c3c" stroke="#c0392b" strokeWidth="1" />
            <text textAnchor="middle" dy=".3em" fill="#fff" fontSize="7" fontWeight="bold" pointerEvents="none">
              {v.id}
            </text>
          </g>
        ))}
      </svg>
      
      {/* Legend */}
      <div style={{ position: 'absolute', bottom: '10px', left: '10px', backgroundColor: 'rgba(255, 255, 255, 0.9)', padding: '10px', borderRadius: '4px', border: '1px solid #ccc', fontSize: '0.8rem', pointerEvents: 'none' }}>
        <strong style={{ display: 'block', marginBottom: '5px' }}>Traffic</strong>
        <div style={{ display: 'flex', alignItems: 'center', marginBottom: '2px' }}><div style={{ width: '20px', height: '4px', backgroundColor: '#bdc3c7', marginRight: '5px' }}></div> Free</div>
        <div style={{ display: 'flex', alignItems: 'center', marginBottom: '2px' }}><div style={{ width: '20px', height: '4px', backgroundColor: '#f1c40f', marginRight: '5px' }}></div> Moderate</div>
        <div style={{ display: 'flex', alignItems: 'center', marginBottom: '10px' }}><div style={{ width: '20px', height: '4px', backgroundColor: '#e67e22', marginRight: '5px' }}></div> Congested</div>
        
        <strong style={{ display: 'block', marginBottom: '5px' }}>Incidents</strong>
        <div style={{ display: 'flex', alignItems: 'center', marginBottom: '2px' }}><div style={{ width: '20px', height: '4px', backgroundColor: '#c0392b', marginRight: '5px' }}></div> Accident</div>
        <div style={{ display: 'flex', alignItems: 'center', marginBottom: '10px' }}><div style={{ width: '20px', height: '4px', borderBottom: '4px dashed #8e44ad', marginRight: '5px' }}></div> Closed</div>
        
        <strong style={{ display: 'block', marginBottom: '5px' }}>Route</strong>
        <div style={{ display: 'flex', alignItems: 'center' }}><div style={{ width: '20px', height: '4px', backgroundColor: '#3498db', marginRight: '5px' }}></div> Active Route</div>
      </div>
    </div>
  );
}
