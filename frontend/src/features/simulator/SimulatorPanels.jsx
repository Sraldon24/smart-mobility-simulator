import {
  ArrowRight,
  BarChart3,
  Car,
  Map,
  Pause,
  Play,
  Route,
  RotateCcw,
} from "lucide-react";
import RoutingControls from "../../components/routing/RoutingControls";
import RouteSummary from "../../components/routing/RouteSummary";
import RecommendationPanel from "../../components/recommendations/RecommendationPanel";
import IncidentPanel from "../../components/traffic/IncidentPanel";
import AlgorithmComparison from "../../components/AlgorithmComparison";
import AnalyticsDashboard from "../../components/AnalyticsDashboard";
import { API_URL } from "./api";

export function AppHeader({ model, onMode }) {
  return (
    <header className="app-header">
      <div className="brand">
        <span className="brand-icon">
          <Route size={23} />
        </span>
        <div>
          <h1>
            Mobility Lab<span className="beta-tag">LIVE DEMO</span>
          </h1>
          <p>Smart Mobility Simulator</p>
        </div>
      </div>
      <div className="city-switch" aria-label="Choose a map">
        <button
          disabled={model.busy}
          aria-pressed={model.mode === "generated"}
          onClick={() => onMode("generated")}
        >
          <Map size={15} />
          Practice grid
        </button>
        <button
          disabled={model.busy}
          aria-pressed={model.mode === "montreal"}
          onClick={() => onMode("montreal")}
        >
          Montréal<span>Downtown</span>
        </button>
      </div>
      <div className="connection">
        <span className={`status-dot ${model.status}`} />
        {model.status === "online"
          ? "Simulator online"
          : model.status === "offline"
            ? "Reconnecting"
            : "Connecting"}
      </div>
    </header>
  );
}

export function Sidebar({
  tab,
  setTab,
  routing,
  model,
  profile,
  setProfile,
  incidentProps,
}) {
  const tabs = [
    ["route", Route, "Route"],
    ["traffic", Car, "Traffic"],
    ["insights", BarChart3, "Insights"],
  ];
  return (
    <aside className="app-sidebar">
      <nav className="workspace-tabs" aria-label="Simulator tools">
        {tabs.map(([id, Icon, label]) => (
          <button key={id} aria-pressed={tab === id} onClick={() => setTab(id)}>
            <Icon size={16} />
            {label}
          </button>
        ))}
      </nav>
      <div className="sidebar-body">
        {tab === "route" && (
          <>
            <RoutingControls {...routing} busy={model.busy} />
            <RouteSummary routeResult={model.route} />
            <details className="advanced-options">
              <summary>Personalized recommendations</summary>
              <RecommendationPanel
                disabled={
                  model.busy ||
                  routing.startNode === null ||
                  routing.destNode === null ||
                  routing.startNode === routing.destNode
                }
                recommendationResult={model.recommendation}
                selectedProfile={profile}
                setSelectedProfile={setProfile}
                onRecommend={() =>
                  model.recommend(routing.startNode, routing.destNode, profile)
                }
                onCandidateSelect={(route) => model.update({ route })}
              />
            </details>
            {model.comparison && (
              <AlgorithmComparison comparisonResults={model.comparison} />
            )}
          </>
        )}
        {tab === "traffic" && (
          <>
            <div className="section-heading">
              <span className="eyebrow">EXPLORE WHAT CHANGES</span>
              <h2>A city in motion</h2>
              <p>
                Add cars with the controls below the map, then advance time to
                watch them move.
              </p>
            </div>
            <div className="traffic-summary">
              <strong>{model.vehicles.length}</strong>
              <span>vehicles in this simulation</span>
            </div>
            <details className="advanced-options">
              <summary>Add a road incident</summary>
              <p className="helper-text">
                Use the point IDs for two connected intersections. Incidents can
                change your route.
              </p>
              <IncidentPanel {...incidentProps} />
            </details>
          </>
        )}
        {tab === "insights" && (
          <>
            <div className="section-heading">
              <span className="eyebrow">LOOK UNDER THE HOOD</span>
              <h2>City insights</h2>
              <p>Live routing, traffic, and simulation performance.</p>
            </div>
            <AnalyticsDashboard apiUrl={API_URL} />
          </>
        )}
      </div>
      <div className="sidebar-footer">
        <span className="legend-dot" />
        Real algorithms. A little room to experiment.
      </div>
    </aside>
  );
}

export function SimulationBar({ model, running, setRunning }) {
  const seconds = Math.floor(model.simulation.simulationTimeSeconds || 0);
  const step = () => model.simulate("step", { deltaTimeSeconds: 1 });
  return (
    <div className="simulation-bar" aria-label="Simulation controls">
      <div className="simulation-caption">
        <Car size={18} />
        <div>
          <strong>{model.vehicles.length} vehicles</strong>
          <small>
            {Math.floor(seconds / 60)}:{String(seconds % 60).padStart(2, "0")}{" "}
            simulated
          </small>
        </div>
      </div>
      <button
        className="btn"
        disabled={model.busy}
        onClick={() => model.simulate("vehicles/batch", { count: 10 })}
      >
        + 10 cars
      </button>
      <button
        className="btn btn-primary play-button"
        disabled={model.busy && !running}
        onClick={() => setRunning(!running)}
      >
        {running ? <Pause size={16} /> : <Play size={16} />}{" "}
        {running ? "Pause" : "Play"}
      </button>
      <button
        className="btn step-button"
        disabled={model.busy || running}
        onClick={step}
      >
        Step +1s <ArrowRight size={14} />
      </button>
      <button
        className="icon-button"
        aria-label="Reset simulation"
        title="Reset simulation"
        disabled={model.busy}
        onClick={() => {
          setRunning(false);
          model.simulate("reset");
        }}
      >
        <RotateCcw size={17} />
      </button>
    </div>
  );
}
