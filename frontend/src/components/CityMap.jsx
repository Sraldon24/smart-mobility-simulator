import { useMemo } from "react";
export default function CityMap({
  nodes,
  roads,
  startNode,
  destNode,
  routeNodeIds,
  vehicles,
  onNodeClick,
}) {
  const nodeIndex = useMemo(
    () => new Map(nodes.map((node) => [node.id, node])),
    [nodes],
  );
  const visualRoads = useMemo(
    () => [
      ...new Map(
        roads.map((road) => [
          [road.from, road.to].sort((a, b) => a - b).join("-"),
          road,
        ]),
      ).values(),
    ],
    [roads],
  );
  const coordinates = routeNodeIds
    .flatMap((id) =>
      nodeIndex.has(id)
        ? [`${nodeIndex.get(id).x},${nodeIndex.get(id).y}`]
        : [],
    )
    .join(" ");
  return (
    <div className="grid-map">
      <svg
        viewBox="-55 -55 510 510"
        role="img"
        aria-label="Practice city map. Choose numbered intersections for your route."
      >
        <defs>
          <pattern
            id="grid-dots"
            width="20"
            height="20"
            patternUnits="userSpaceOnUse"
          >
            <circle cx="0" cy="0" r="0.8" fill="#dbe3dd" />
          </pattern>
        </defs>
        <rect x="-55" y="-55" width="510" height="510" fill="url(#grid-dots)" />
        {[0, 1, 2, 3].flatMap((x) =>
          [0, 1, 2, 3].map((y) => (
            <rect
              key={`${x}-${y}`}
              x={x * 100 + 18}
              y={y * 100 + 18}
              width="64"
              height="64"
              rx="12"
              fill={(x + y) % 3 === 0 ? "#e0ecdf" : "#e9eae3"}
            />
          )),
        )}
        {visualRoads.map((road) => {
          const a = nodeIndex.get(road.from),
            b = nodeIndex.get(road.to);
          if (!a || !b) return null;
          return (
            <line
              key={`${road.from}-${road.to}`}
              x1={a.x}
              y1={a.y}
              x2={b.x}
              y2={b.y}
              stroke={
                road.closed
                  ? "#c64d48"
                  : road.trafficFactor >= 2
                    ? "#d89836"
                    : "#d1d8d3"
              }
              strokeWidth="9"
              strokeLinecap="round"
              strokeDasharray={road.closed ? "5 5" : undefined}
            />
          );
        })}
        {coordinates && (
          <polyline
            points={coordinates}
            fill="none"
            stroke="#087f68"
            strokeWidth="8"
            strokeLinecap="round"
            strokeLinejoin="round"
          />
        )}
        {nodes.map((node) => (
          <g
            key={node.id}
            role="button"
            tabIndex="0"
            aria-label={`Select point ${node.id}`}
            onClick={() => onNodeClick(node.id)}
            onKeyDown={(event) => {
              if (event.key === "Enter" || event.key === " ") {
                event.preventDefault();
                onNodeClick(node.id);
              }
            }}
            className="grid-node"
          >
            <circle cx={node.x} cy={node.y} r="18" fill="transparent" />
            <circle
              cx={node.x}
              cy={node.y}
              r="12"
              fill={
                node.id === startNode
                  ? "#087f68"
                  : node.id === destNode
                    ? "#e17c50"
                    : "#fff"
              }
              stroke={
                node.id === startNode || node.id === destNode
                  ? "#fff"
                  : "#b9c9bf"
              }
              strokeWidth="2"
            />
            <text
              x={node.x}
              y={node.y + 3.5}
              textAnchor="middle"
              fontSize="10"
              fontWeight="600"
              fill={
                node.id === startNode || node.id === destNode
                  ? "#fff"
                  : "#52675c"
              }
            >
              {node.id}
            </text>
          </g>
        ))}
        {vehicles.map((vehicle) => (
          <circle
            key={vehicle.id}
            cx={vehicle.x}
            cy={vehicle.y}
            r="3.5"
            fill="#356dbe"
            stroke="#fff"
            strokeWidth="1"
            pointerEvents="none"
          />
        ))}
      </svg>
    </div>
  );
}
