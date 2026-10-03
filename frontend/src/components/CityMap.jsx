import React from 'react';

export default function CityMap({ nodes, roads, startNode, destNode, routeNodeIds, onNodeClick }) {
  // Deduplicate roads visually
  const visualRoads = [];
  const seenRoads = new Set();
  
  roads.forEach(r => {
    const min = Math.min(r.from, r.to);
    const max = Math.max(r.from, r.to);
    const key = `${min}-${max}`;
    if (!seenRoads.has(key)) {
      seenRoads.add(key);
      visualRoads.push(r);
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

  return (
    <div style={{ width: '100%', display: 'flex', justifyContent: 'center' }}>
      <svg 
        viewBox="-50 -50 500 500" 
        style={{ width: '100%', maxWidth: '600px', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6' }}
      >
        {/* Draw all roads */}
        {visualRoads.map(r => {
          const min = Math.min(r.from, r.to);
          const max = Math.max(r.from, r.to);
          const key = `${min}-${max}`;
          
          const isRoute = routeSegments.has(key);
          const n1 = nodeMap.get(r.from);
          const n2 = nodeMap.get(r.to);
          
          if (!n1 || !n2) return null;

          return (
            <line
              key={key}
              x1={n1.x}
              y1={n1.y}
              x2={n2.x}
              y2={n2.y}
              stroke={isRoute ? '#3498db' : '#bdc3c7'}
              strokeWidth={isRoute ? 10 : 4}
              strokeLinecap="round"
            />
          );
        })}

        {/* Draw all nodes */}
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
              <circle 
                r={r} 
                fill={fill} 
                stroke={stroke} 
                strokeWidth={3} 
                // Add a hover effect wrapper conceptually
              />
              <text 
                textAnchor="middle" 
                dy=".3em" 
                fill="#ffffff" 
                fontSize={isStart || isDest ? "14" : "12"}
                fontWeight="bold"
                pointerEvents="none"
              >
                {n.id}
              </text>
            </g>
          );
        })}
      </svg>
    </div>
  );
}
