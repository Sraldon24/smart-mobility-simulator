import { useState, useEffect, useRef } from 'react';
import CityMap from './components/CityMap';
import MontrealMap from './components/MontrealMap';
import AnalyticsDashboard from './components/AnalyticsDashboard';
import RoutingControls from './components/routing/RoutingControls';
import RouteSummary from './components/routing/RouteSummary';
import SimulationControls from './components/simulation/SimulationControls';
import VehicleMetrics from './components/simulation/VehicleMetrics';
import IncidentPanel from './components/traffic/IncidentPanel';
import RecommendationPanel from './components/recommendations/RecommendationPanel';
import AlgorithmComparison from './components/AlgorithmComparison';
import { Activity, Map as MapIcon, Database } from 'lucide-react';
import './App.css';

export default function App() {
  const [cityMode, setCityMode] = useState('generated');
  const [isLoadingMode, setIsLoadingMode] = useState(false);
  const [cityData, setCityData] = useState({ nodes: [], roads: [] });
  const [incidents, setIncidents] = useState([]);
  
  const [startNode, setStartNode] = useState(0);
  const [destNode, setDestNode] = useState(24);
  const [algorithm, setAlgorithm] = useState('astar');
  const [objective, setObjective] = useState('fastest');
  
  const [routeResult, setRouteResult] = useState(null);
  const [routeError, setRouteError] = useState(null);
  
  const [incidentFrom, setIncidentFrom] = useState('');
  const [incidentTo, setIncidentTo] = useState('');
  const [incidentType, setIncidentType] = useState('closure');
  const [incidentError, setIncidentError] = useState(null);
  const [rerouteStatus, setRerouteStatus] = useState(null);

  const [vehicles, setVehicles] = useState([]);
  const [simulationTime, setSimulationTime] = useState(0);
  const [congestedRoadCount, setCongestedRoadCount] = useState(0);
  const [averageCongestion, setAverageCongestion] = useState(1.0);
  const [totalReroutes, setTotalReroutes] = useState(0);

  const [recommendationResult, setRecommendationResult] = useState(null);
  const [selectedProfile, setSelectedProfile] = useState('balanced');
  
  const [comparisonResults, setComparisonResults] = useState(null);

  const API_URL = 'http://localhost:8400';

  useEffect(() => {
    fetchCityAndIncidents();
  }, [cityMode]);

  useEffect(() => {
    const interval = setInterval(() => {
      fetchSimulationState();
    }, 1000); // Polling every second for UI refresh
    return () => clearInterval(interval);
  }, []);

  const fetchCityAndIncidents = () => {
    fetch(`${API_URL}/city`)
      .then(res => res.json())
      .then(data => {
        setCityData(data);
        setIsLoadingMode(false);
      })
      .catch(err => {
        console.error("Failed to load city:", err);
        setIsLoadingMode(false);
      });

    fetch(`${API_URL}/incidents`)
      .then(res => res.json())
      .then(data => setIncidents(data.incidents || []))
      .catch(err => console.error("Failed to load incidents:", err));
      
    fetchSimulationState();
  };

  const fetchSimulationState = () => {
    fetch(`${API_URL}/simulation`)
      .then(res => res.json())
      .then(data => {
        setSimulationTime(data.simulationTimeSeconds || 0);
        setCongestedRoadCount(data.congestedRoadCount || 0);
        setAverageCongestion(data.averageCongestionFactor || 1.0);
        setTotalReroutes(data.totalReroutes || 0);
      })
      .catch(err => {});

    fetch(`${API_URL}/vehicles`)
      .then(res => res.json())
      .then(data => setVehicles(data || []))
      .catch(err => {});
  };

  const handleCityModeChange = (mode) => {
    if (mode === cityMode) return;
    setCityMode(mode);
    setIsLoadingMode(true);
    setRouteResult(null);
    setRecommendationResult(null);
    setComparisonResults(null);
    setRerouteStatus(null);
    setStartNode(null);
    setDestNode(null);
    
    fetch(`${API_URL}/mode`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ mode })
    }).then(() => fetchCityAndIncidents());
  };

  const handleNodeClick = (nodeId) => {
    if (startNode === null) {
      setStartNode(nodeId);
    } else if (destNode === null) {
      setDestNode(nodeId);
    } else {
      setStartNode(nodeId);
      setDestNode(null);
    }
  };

  const handleFindRoute = () => {
    if (startNode === null || destNode === null) return;
    setRouteError(null);
    setRerouteStatus(null);
    setRecommendationResult(null);
    setComparisonResults(null);

    fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=${algorithm}&objective=${objective}`)
      .then(res => res.json().then(data => ({ status: res.status, data })))
      .then(({ status, data }) => {
        if (status === 200 && data.found) setRouteResult(data);
        else if (status === 404) setRouteResult({ found: false });
        else setRouteError(data.error?.message || "Routing failed");
      })
      .catch(err => setRouteError(err.message));
  };

  const handleCompare = () => {
    if (startNode === null || destNode === null) return;
    Promise.all([
      fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=dijkstra&objective=${objective}`).then(r => r.json()),
      fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=astar&objective=${objective}`).then(r => r.json())
    ]).then(([dijkstraData, astarData]) => {
      setComparisonResults({ dijkstra: dijkstraData, astar: astarData });
      if (astarData.found) setRouteResult(astarData);
    });
  };

  const handleRecommendRoute = () => {
    if (startNode === null || destNode === null) return;
    setRouteError(null);
    setComparisonResults(null);
    
    fetch(`${API_URL}/recommend-route?start=${startNode}&end=${destNode}&profile=${selectedProfile}`)
      .then(res => res.json().then(data => ({ status: res.status, data })))
      .then(({ status, data }) => {
        if (status === 200) {
          setRecommendationResult(data);
          setRouteResult({
            found: true,
            nodeIds: data.recommendedRoute.nodeIds,
            totalDistanceMeters: data.recommendedRoute.distanceMeters,
            estimatedTravelTimeSeconds: data.recommendedRoute.travelTimeSeconds,
            algorithm: 'Recommendation',
            objective: data.recommendedRoute.sourceObjective
          });
        }
      });
  };

  const handleAddIncident = () => {
    if (incidentFrom === '' || incidentTo === '') return;
    setIncidentError(null);
    
    const body = {
      from: parseInt(incidentFrom),
      to: parseInt(incidentTo),
      type: incidentType
    };

    if (routeResult && routeResult.found) {
      body.activeRoute = {
        start: startNode, end: destNode, algorithm, objective,
        nodeIds: routeResult.nodeIds,
        totalDistanceMeters: routeResult.totalDistanceMeters,
        estimatedTravelTimeSeconds: routeResult.estimatedTravelTimeSeconds
      };
    }

    fetch(`${API_URL}/incident`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    }).then(res => res.json()).then(data => {
      if (data.status === 'success') {
        fetchCityAndIncidents();
        setIncidentFrom('');
        setIncidentTo('');
        if (data.rerouted && data.newRouteResult) {
          setRouteResult(data.newRouteResult);
          setRerouteStatus({ type: 'error', message: 'Route updated due to new incident.', ...data });
        }
      } else {
        setIncidentError(data.error?.message);
      }
    });
  };

  const handleRemoveIncident = (from, to) => {
    const body = { from, to };
    if (routeResult && routeResult.found) {
      body.activeRoute = {
        start: startNode, end: destNode, algorithm, objective,
        nodeIds: routeResult.nodeIds,
        totalDistanceMeters: routeResult.totalDistanceMeters,
        estimatedTravelTimeSeconds: routeResult.estimatedTravelTimeSeconds
      };
    }

    fetch(`${API_URL}/incident`, {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    }).then(res => res.json()).then(data => {
      if (data.status === 'success') {
        fetchCityAndIncidents();
        if (data.rerouted && data.newRouteResult) {
          setRouteResult(data.newRouteResult);
          setRerouteStatus({ type: 'info', message: 'Route improved after incident cleared.', ...data });
        }
      }
    });
  };

  const handleSpawnBatch = (count) => {
    fetch(`${API_URL}/simulation/vehicles/batch`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ count })
    }).then(() => fetchSimulationState());
  };

  const handleStepSimulation = (dt) => {
    fetch(`${API_URL}/simulation/step`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ deltaTimeSeconds: dt })
    }).then(() => fetchSimulationState());
  };

  const handleResetSimulation = () => {
    fetch(`${API_URL}/simulation/reset`, { method: 'POST' }).then(() => fetchSimulationState());
  };

  return (
    <div className="app-container">
      {/* Header */}
      <header className="app-header">
        <div className="flex-row gap-md">
          <Activity color="var(--accent)" />
          <h1 style={{ color: 'var(--text-primary)' }}>Smart Mobility Simulator</h1>
        </div>
        
        <div className="segmented-control" style={{ width: '300px' }}>
          <button className={cityMode === 'generated' ? 'active' : ''} onClick={() => handleCityModeChange('generated')}>Generated Grid</button>
          <button className={cityMode === 'montreal' ? 'active' : ''} onClick={() => handleCityModeChange('montreal')}>Montreal OSM</button>
        </div>

        <div className="header-status">
          <span className={`status-dot ${cityData.nodes.length > 0 ? 'online' : 'loading'}`}></span>
          <span className="text-muted">{cityData.nodes.length > 0 ? 'Backend Connected' : 'Connecting...'}</span>
        </div>
      </header>

      {/* Main Layout */}
      <main className="app-main">
        {/* Sidebar */}
        <aside className="app-sidebar">
          <div className="sidebar-section">
            <RoutingControls 
              startNode={startNode} setStartNode={setStartNode}
              destNode={destNode} setDestNode={setDestNode}
              algorithm={algorithm} setAlgorithm={setAlgorithm}
              objective={objective} setObjective={setObjective}
              onFindRoute={handleFindRoute}
              onCompare={handleCompare}
            />
            <div style={{ marginTop: '1rem' }}>
              <RouteSummary routeResult={routeResult} routeError={routeError} rerouteStatus={rerouteStatus} />
            </div>
            {comparisonResults && (
              <div style={{ marginTop: '1rem' }}>
                <AlgorithmComparison comparisonResults={comparisonResults} comparisonError={null} />
              </div>
            )}
          </div>

          <div className="sidebar-section">
            <RecommendationPanel 
              recommendationResult={recommendationResult}
              selectedProfile={selectedProfile}
              setSelectedProfile={setSelectedProfile}
              onRecommend={handleRecommendRoute}
              onCandidateSelect={(res) => setRouteResult(res)}
            />
          </div>

          <div className="sidebar-section">
            <IncidentPanel 
              incidents={incidents}
              incidentFrom={incidentFrom} setIncidentFrom={setIncidentFrom}
              incidentTo={incidentTo} setIncidentTo={setIncidentTo}
              incidentType={incidentType} setIncidentType={setIncidentType}
              onAddIncident={handleAddIncident}
              onRemoveIncident={handleRemoveIncident}
              incidentError={incidentError}
            />
          </div>

          <div className="sidebar-section">
            <SimulationControls 
              onStep={handleStepSimulation}
              onReset={handleResetSimulation}
              onSpawnBatch={handleSpawnBatch}
            />
            <div style={{ marginTop: '1rem' }}>
              <VehicleMetrics 
                simulationTime={simulationTime}
                congestedRoadCount={congestedRoadCount}
                averageCongestion={averageCongestion}
                totalReroutes={totalReroutes}
                vehicles={vehicles}
              />
            </div>
          </div>
        </aside>

        {/* Map Area */}
        <section className="map-container">
          {isLoadingMode ? (
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', height: '100%', color: 'var(--text-muted)' }}>
              Loading Map Data...
            </div>
          ) : cityData.nodes.length > 0 ? (
            cityMode === 'montreal' ? (
              <MontrealMap 
                nodes={cityData.nodes} 
                roads={cityData.roads} 
                incidents={incidents}
                startNode={startNode} 
                destNode={destNode}
                routeNodeIds={routeResult?.found ? routeResult.nodeIds : []}
                vehicles={vehicles}
                onNodeClick={handleNodeClick}
              />
            ) : (
              <CityMap 
                nodes={cityData.nodes} 
                roads={cityData.roads} 
                incidents={incidents}
                startNode={startNode} 
                destNode={destNode}
                routeNodeIds={routeResult?.found ? routeResult.nodeIds : []}
                vehicles={vehicles}
                onNodeClick={handleNodeClick}
              />
            )
          ) : null}

          {/* Analytics Overlay */}
          <div className="analytics-overlay">
            <AnalyticsDashboard />
          </div>
        </section>
      </main>
    </div>
  );
}
