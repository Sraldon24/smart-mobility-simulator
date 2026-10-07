import { useEffect, useState } from "react";
import { MousePointer2, X } from "lucide-react";
import CityMap from "./components/CityMap";
import MontrealMap from "./components/MontrealMap";
import { useSimulator } from "./features/simulator/useSimulator";
import {
  AppHeader,
  Sidebar,
  SimulationBar,
} from "./features/simulator/SimulatorPanels";
import "./App.css";

function nearest(nodes, lon, lat) {
  return nodes.reduce((best, node) =>
    Math.hypot(node.x - lon, node.y - lat) <
    Math.hypot(best.x - lon, best.y - lat)
      ? node
      : best,
  ).id;
}

export default function App() {
  const model = useSimulator();
  const [tab, setTab] = useState("route");
  const [startNode, setStartNode] = useState(null);
  const [destNode, setDestNode] = useState(null);
  const [selecting, setSelecting] = useState("start");
  const [algorithm, setAlgorithm] = useState("astar");
  const [objective, setObjective] = useState("fastest");
  const [profile, setProfile] = useState("balanced");
  const [running, setRunning] = useState(false);
  const [incidentFrom, setIncidentFrom] = useState("");
  const [incidentTo, setIncidentTo] = useState("");
  const [incidentType, setIncidentType] = useState("closure");

  useEffect(() => {
    if (!running || model.busy) return;
    const timer = setTimeout(
      () => model.simulate("step", { deltaTimeSeconds: 1 }),
      1000,
    );
    return () => clearTimeout(timer);
  }, [running, model]);

  const onMode = async (mode) => {
    if (mode === model.mode && model.city.nodes.length) return;
    setRunning(false);
    if (await model.changeMode(mode)) {
      setStartNode(null);
      setDestNode(null);
      setSelecting("start");
    }
  };
  const selectNode = (id) => {
    model.update({ route: null, comparison: null, recommendation: null });
    if (selecting === "start") {
      setStartNode(id);
      setSelecting("end");
    } else {
      setDestNode(id);
      setSelecting("start");
    }
    setTab("route");
  };
  const example = () => {
    if (!model.city.nodes.length) return;
    const start =
      model.mode === "generated"
        ? 0
        : nearest(model.city.nodes, -73.5785, 45.4971);
    const end =
      model.mode === "generated"
        ? 24
        : nearest(model.city.nodes, -73.556, 45.503);
    setStartNode(start);
    setDestNode(end);
    setSelecting("start");
    model.findRoute(start, end, algorithm, objective);
  };
  const selection = { start: startNode, end: destNode, algorithm, objective };
  const changePoint = (setter, value) => {
    setter(value);
    model.update({ route: null, comparison: null, recommendation: null });
  };
  const routing = {
    startNode,
    setStartNode: (value) => changePoint(setStartNode, value),
    destNode,
    setDestNode: (value) => changePoint(setDestNode, value),
    algorithm,
    setAlgorithm,
    objective,
    setObjective,
    selecting,
    onSelect: setSelecting,
    onExample: example,
    onFindRoute: () =>
      model.findRoute(startNode, destNode, algorithm, objective),
    onCompare: () => model.compare(startNode, destNode, objective),
  };
  const incidentProps = {
    incidents: model.incidents,
    incidentFrom,
    setIncidentFrom,
    incidentTo,
    setIncidentTo,
    incidentType,
    setIncidentType,
    onAddIncident: () =>
      model.incident(
        "POST",
        {
          from: Number(incidentFrom),
          to: Number(incidentTo),
          type: incidentType,
        },
        selection,
      ),
    onRemoveIncident: (from, to) =>
      model.incident("DELETE", { from, to }, selection),
  };
  const MapComponent = model.mode === "montreal" ? MontrealMap : CityMap;

  return (
    <div className="app-container">
      <AppHeader model={model} onMode={onMode} />
      {(model.error || model.message) && (
        <div
          className={`app-notice ${model.error ? "error" : ""}`}
          role="alert"
        >
          <span>{model.error || model.message}</span>
          <button
            aria-label="Dismiss message"
            onClick={() => model.update({ error: "", message: "" })}
          >
            <X size={17} />
          </button>
        </div>
      )}
      <main className="app-main">
        <Sidebar
          {...{
            tab,
            setTab,
            routing,
            model,
            profile,
            setProfile,
            incidentProps,
          }}
        />
        <section className="map-workspace" aria-label="Interactive city map">
          <div className="map-heading">
            <div>
              <span className="eyebrow">
                {model.mode === "montreal"
                  ? "45.5017° N · 73.5673° W"
                  : "YOUR ROUTING SANDBOX"}
              </span>
              <h2>
                {model.mode === "montreal"
                  ? "A little Montréal. A lot to discover."
                  : "Every journey starts with a point."}
              </h2>
            </div>
            <span className="map-badge">
              {model.mode === "montreal"
                ? "OpenStreetMap · Downtown extract"
                : "5 × 5 practice grid"}
            </span>
          </div>
          <div className="map-container">
            <MapComponent
              nodes={model.city.nodes}
              roads={model.city.roads}
              startNode={startNode}
              destNode={destNode}
              routeNodeIds={model.route?.found ? model.route.nodeIds : []}
              vehicles={model.vehicles}
              onNodeClick={selectNode}
            />
            <div className="map-instruction">
              <MousePointer2 size={16} />
              {selecting === "start"
                ? "Click a point to choose your start"
                : "Now choose your destination"}
              <span className="key-cap">
                {selecting === "start" ? "1" : "2"}
              </span>
            </div>
            {(model.busy || !model.city.nodes.length) && (
              <div className="map-loading" role="status">
                {model.busy
                  ? "Updating your city…"
                  : model.status === "offline"
                    ? "The simulator is reconnecting. Please wait."
                    : "Loading your map…"}
              </div>
            )}
            <div className="map-legend">
              <span>
                <i className="legend-dot start" />
                Start
              </span>
              <span>
                <i className="legend-dot end" />
                Destination
              </span>
              <span>
                <i className="legend-line" />
                Your route
              </span>
            </div>
          </div>
          <SimulationBar {...{ model, running, setRunning }} />
        </section>
      </main>
    </div>
  );
}
