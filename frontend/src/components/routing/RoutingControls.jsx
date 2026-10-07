import {
  ArrowRight,
  MapPin,
  Navigation,
  RotateCcw,
  Sparkles,
} from "lucide-react";

export default function RoutingControls({
  startNode,
  setStartNode,
  destNode,
  setDestNode,
  algorithm,
  setAlgorithm,
  objective,
  setObjective,
  onFindRoute,
  onCompare,
  selecting,
  onSelect,
  onExample,
  busy,
}) {
  const ready =
    startNode !== null && destNode !== null && startNode !== destNode;
  return (
    <div className="route-controls">
      <div className="section-heading">
        <span className="eyebrow">PLAN YOUR JOURNEY</span>
        <h2>Where are we going?</h2>
        <p>Pick two points on the map. We’ll find the way.</p>
      </div>
      <div className="endpoint-fields">
        <button
          className={`endpoint ${selecting === "start" ? "selecting" : ""}`}
          onClick={() => onSelect("start")}
        >
          <span className="endpoint-dot start">
            <MapPin size={17} />
          </span>
          <span>
            <small>Starting point</small>
            <strong>
              {startNode === null ? "Click on the map" : `Point ${startNode}`}
            </strong>
          </span>
          <span className="edit-label">Choose</span>
        </button>
        <button
          className={`endpoint ${selecting === "end" ? "selecting" : ""}`}
          onClick={() => onSelect("end")}
        >
          <span className="endpoint-dot end">
            <Navigation size={17} />
          </span>
          <span>
            <small>Destination</small>
            <strong>
              {destNode === null ? "Click on the map" : `Point ${destNode}`}
            </strong>
          </span>
          <span className="edit-label">Choose</span>
        </button>
      </div>
      <button
        className="text-button"
        onClick={() => {
          setStartNode(null);
          setDestNode(null);
          onSelect("start");
        }}
      >
        <RotateCcw size={13} /> Clear points
      </button>
      <label className="field-label" htmlFor="route-preference">
        What matters most?
      </label>
      <select
        id="route-preference"
        value={objective}
        onChange={(event) => setObjective(event.target.value)}
      >
        <option value="fastest">Get there faster</option>
        <option value="shortest">Travel less distance</option>
        <option value="least_traffic">Avoid busy roads</option>
      </select>
      <button
        className="btn btn-primary route-submit"
        disabled={!ready || busy}
        onClick={onFindRoute}
      >
        {busy ? "Working…" : "Find my route"}
        <ArrowRight size={18} />
      </button>
      {!ready && (
        <p className="helper-text">
          {startNode === destNode && startNode !== null
            ? "Choose a different destination."
            : "Choose a start and destination to continue."}
        </p>
      )}
      <button className="example-button" onClick={onExample} disabled={busy}>
        <Sparkles size={16} /> Try a sample journey
      </button>
      <details className="advanced-options">
        <summary>Advanced route options</summary>
        <label className="field-label" htmlFor="route-algorithm">
          Routing algorithm
        </label>
        <select
          id="route-algorithm"
          value={algorithm}
          onChange={(event) => setAlgorithm(event.target.value)}
        >
          <option value="astar">A* (recommended)</option>
          <option value="dijkstra">Dijkstra</option>
        </select>
        <div className="manual-points">
          <label>
            Start ID
            <input
              type="number"
              value={startNode ?? ""}
              onChange={(event) =>
                setStartNode(
                  event.target.value === "" ? null : Number(event.target.value),
                )
              }
            />
          </label>
          <label>
            Destination ID
            <input
              type="number"
              value={destNode ?? ""}
              onChange={(event) =>
                setDestNode(
                  event.target.value === "" ? null : Number(event.target.value),
                )
              }
            />
          </label>
        </div>
        <button className="btn" disabled={!ready || busy} onClick={onCompare}>
          Compare algorithms
        </button>
      </details>
    </div>
  );
}
