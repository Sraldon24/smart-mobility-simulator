import { useState, useEffect } from 'react'
import CityMap from './components/CityMap'
import MontrealMap from './components/MontrealMap'

function App() {
  const [cityMode, setCityMode] = useState('generated')
  const [isLoadingMode, setIsLoadingMode] = useState(false)
  const [backendStatus, setBackendStatus] = useState('Checking...')
  const [cityData, setCityData] = useState({ nodes: [], roads: [] })
  const [incidents, setIncidents] = useState([])
  
  const [startNode, setStartNode] = useState(0)
  const [destNode, setDestNode] = useState(24)
  
  const [routeResult, setRouteResult] = useState(null)
  const [routeError, setRouteError] = useState(null)

  const [comparisonResults, setComparisonResults] = useState(null)
  const [isComparing, setIsComparing] = useState(false)
  const [comparisonError, setComparisonError] = useState(null)
  
  const [incidentFrom, setIncidentFrom] = useState('')
  const [incidentTo, setIncidentTo] = useState('')
  const [incidentType, setIncidentType] = useState('closure')
  const [incidentError, setIncidentError] = useState(null)
  const [rerouteStatus, setRerouteStatus] = useState(null)

  const [vehicles, setVehicles] = useState([])
  const [simulationTime, setSimulationTime] = useState(0)
  const [congestedRoadCount, setCongestedRoadCount] = useState(0)
  const [averageCongestion, setAverageCongestion] = useState(1.0)
  const [totalReroutes, setTotalReroutes] = useState(0)
  const [simOrigin, setSimOrigin] = useState('')
  const [simDest, setSimDest] = useState('')
  const [simPreference, setSimPreference] = useState('balanced')
  const [simError, setSimError] = useState(null)
  const [simPreferencesStats, setSimPreferencesStats] = useState({})

  const [profiles, setProfiles] = useState([])
  const [selectedProfile, setSelectedProfile] = useState('balanced')
  const [profileWeights, setProfileWeights] = useState(null)

  const API_URL = 'http://localhost:8400'

  const fetchCityAndIncidents = () => {
    fetch(`${API_URL}/city`)
      .then(res => res.json())
      .then(data => setCityData(data))
      .catch(err => console.error("Failed to load city:", err))

    fetch(`${API_URL}/incidents`)
      .then(res => res.json())
      .then(data => setIncidents(data.incidents || []))
      .catch(err => console.error("Failed to load incidents:", err))
      
    fetchSimulationState()
  }

  const fetchSimulationState = () => {
    fetch(`${API_URL}/simulation`)
      .then(res => res.json())
      .then(data => {
        setSimulationTime(data.simulationTimeSeconds || 0)
        setCongestedRoadCount(data.congestedRoadCount || 0)
        setAverageCongestion(data.averageCongestionFactor || 1.0)
        setTotalReroutes(data.totalReroutes || 0)
        setSimPreferencesStats(data.preferences || {})
      })
      .catch(err => console.error("Failed to load simulation time:", err))

    fetch(`${API_URL}/vehicles`)
      .then(res => res.json())
      .then(data => setVehicles(data || []))
      .catch(err => console.error("Failed to load vehicles:", err))
  }

  const fetchProfiles = () => {
    fetch(`${API_URL}/profiles`)
      .then(res => res.json())
      .then(data => setProfiles(data.profiles || []))
      .catch(err => console.error("Failed to load profiles:", err))
  }

  const handleCityModeChange = (mode) => {
    setCityMode(mode)
    setIsLoadingMode(true)
    setRouteResult(null)
    setComparisonResults(null)
    setRecommendationResult(null)
    setRerouteStatus(null)
    setStartNode(null)
    setDestNode(null)
    
    fetch(`${API_URL}/mode`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ mode })
    })
    .then(async res => {
      const data = await res.json()
      if (!res.ok) throw new Error(data.error)
      fetchCityAndIncidents()
    })
    .catch(err => {
      alert("Failed to change mode: " + err.message)
    })
    .finally(() => {
      setIsLoadingMode(false)
    })
  }

  useEffect(() => {
    fetch(`${API_URL}/health`)
      .then(res => res.json())
      .then(data => {
        if (data.status === 'ok') setBackendStatus('Connected 🟢')
        else setBackendStatus('Error 🔴')
      })
      .catch(() => setBackendStatus('Disconnected 🔴'))

    fetchCityAndIncidents()
    fetchProfiles()
  }, [])

  useEffect(() => {
    if (selectedProfile) {
      fetch(`${API_URL}/profiles/${selectedProfile}`)
        .then(res => res.json())
        .then(data => setProfileWeights(data))
        .catch(err => console.error("Failed to load profile weights:", err))
    }
  }, [selectedProfile])

  const handleSpawnVehicle = () => {
    setSimError(null);
    if (simOrigin === '' || simDest === '') {
      setSimError("Please provide both origin and destination.");
      return;
    }
    
    fetch(`${API_URL}/simulation/vehicle`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        origin: parseInt(simOrigin),
        destination: parseInt(simDest),
        preference: simPreference
      })
    })
    .then(async res => {
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Failed to spawn vehicle');
      
      setSimOrigin('');
      setSimDest('');
      fetchSimulationState();
    })
    .catch(err => setSimError(err.message));
  }

  const handleResetSimulation = () => {
    fetch(`${API_URL}/simulation/reset`, { method: 'POST' })
      .then(() => fetchSimulationState())
      .catch(err => console.error(err));
  }

  const handleStepSimulation = (dt) => {
    fetch(`${API_URL}/simulation/step`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ deltaTimeSeconds: dt })
    })
    .then(async res => {
      const data = await res.json();
      if (!res.ok) throw new Error(data.error);
      setSimulationTime(data.simulationTimeSeconds || 0);
      setVehicles(data.vehicles || []);
    })
    .catch(err => console.error("Step failed:", err));
  }

  const handleSpawnBatch = (count) => {
    fetch(`${API_URL}/simulation/vehicles/batch`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ count })
    })
    .then(async res => {
      const data = await res.json();
      if (!res.ok) throw new Error(data.error);
      fetchSimulationState();
      fetchCityData();
    })
    .catch(err => setSimError(err.message));
  }

  const [algorithm, setAlgorithm] = useState('dijkstra')
  const [objective, setObjective] = useState('shortest')

  const [recommendationResult, setRecommendationResult] = useState(null)

  const handleRecommendRoute = () => {
    if (startNode === null || destNode === null) {
      setRouteError("Please select both a Start and Destination node.");
      return;
    }

    setRouteResult(null)
    setRouteError(null)
    setComparisonResults(null)
    setComparisonError(null)
    setRerouteStatus(null)
    setRecommendationResult(null)
    setProfileComparison(null)

    fetch(`${API_URL}/recommend-route?start=${startNode}&end=${destNode}&profile=${selectedProfile}`)
      .then(async (res) => {
        const data = await res.json()
        if (!res.ok) {
          throw new Error(data.error || 'Failed to recommend route')
        }
        setRecommendationResult(data)
        setRouteResult({
          found: true,
          nodeIds: data.recommendedRoute.nodeIds,
          totalDistanceMeters: data.recommendedRoute.distanceMeters,
          estimatedTravelTimeSeconds: data.recommendedRoute.travelTimeSeconds,
          algorithm: data.recommendedRoute.sourceObjective,
          objective: data.recommendedRoute.sourceObjective
        })
      })
      .catch(err => {
        setRouteError(err.message)
      })
  }

  const [profileComparison, setProfileComparison] = useState(null)
  const [isComparingProfiles, setIsComparingProfiles] = useState(false)

  const handleCompareProfiles = () => {
    if (startNode === null || destNode === null) {
      setRouteError("Please select both a Start and Destination node.");
      return;
    }

    setProfileComparison(null);
    setIsComparingProfiles(true);
    setRouteError(null);
    setRecommendationResult(null);
    setComparisonResults(null);

    fetch(`${API_URL}/recommend-route/compare?start=${startNode}&end=${destNode}`)
      .then(async (res) => {
        const data = await res.json()
        if (!res.ok) throw new Error(data.error || 'Failed to compare profiles')
        setProfileComparison(data)
      })
      .catch(err => setRouteError(err.message))
      .finally(() => setIsComparingProfiles(false))
  }

  const handleFindRoute = (overrideAlgo = algorithm, overrideObj = objective) => {
    if (startNode === null || destNode === null) {
      setRouteError("Please select both a Start and Destination node.");
      return;
    }

    setRouteResult(null)
    setRouteError(null)
    setComparisonResults(null)
    setComparisonError(null)
    setRerouteStatus(null)
    setProfileComparison(null)

    fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=${overrideAlgo}&objective=${overrideObj}`)
      .then(async (res) => {
        const data = await res.json()
        if (!res.ok) {
          throw new Error(data.error || 'Failed to find route')
        }
        setRouteResult(data)
      })
      .catch(err => {
        setRouteError(err.message)
      })
  }

  const handleCompareAlgorithms = async () => {
    if (startNode === null || destNode === null) {
      setComparisonError("Please select both a Start and Destination node.");
      return;
    }

    setIsComparing(true);
    setComparisonError(null);
    setComparisonResults(null);
    setRouteError(null);
    setRerouteStatus(null)
    setProfileComparison(null)

    try {
      const pDijkstra = fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=dijkstra&objective=${objective}`)
        .then(res => res.json().then(data => ({ res, data })))
        .catch(err => ({ error: err.message }));

      const pAStar = fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=astar&objective=${objective}`)
        .then(res => res.json().then(data => ({ res, data })))
        .catch(err => ({ error: err.message }));

      const [dResult, aResult] = await Promise.all([pDijkstra, pAStar]);

      const processResult = (result) => {
        if (result.error) return { failed: true, error: result.error };
        if (!result.res.ok) return { failed: true, error: result.data.error || 'Failed' };
        return result.data;
      };

      const dijkstraData = processResult(dResult);
      const astarData = processResult(aResult);

      setComparisonResults({
        dijkstra: dijkstraData,
        astar: astarData
      });

      if (!dijkstraData.failed) {
        setRouteResult(dijkstraData);
      } else if (!astarData.failed) {
        setRouteResult(astarData);
      }
    } catch (err) {
      setComparisonError("Comparison failed: " + err.message);
    } finally {
      setIsComparing(false);
    }
  }

  const handleNodeClick = (id) => {
    if (startNode !== null && destNode !== null) {
      setStartNode(id);
      setDestNode(null);
      setRouteResult(null);
      setRouteError(null);
      setComparisonResults(null);
      setComparisonError(null);
      setRerouteStatus(null);
    } else if (startNode === null) {
      setStartNode(id);
    } else {
      setDestNode(id);
    }
  };

  const processRerouteResponse = (data, isClear = false, from = null, to = null, type = null) => {
    if (data.routeAffected) {
      if (data.rerouted) {
        if (!data.newRouteResult.found) {
          setRerouteStatus({ type: 'error', message: `No alternative route available.` });
        } else {
          let msg = "Route recalculated to avoid incident.";
          if (isClear) {
            msg = `Better route found after incident cleared on ${from} ↔ ${to}.`;
          } else if (type === 'closure') {
            msg = `Route recalculated: road ${from} ↔ ${to} is closed.`;
          } else if (type === 'accident') {
            msg = `Faster route selected after accident on ${from} ↔ ${to}.`;
          }
          
          setRerouteStatus({ 
            type: 'success', 
            message: msg,
            previousDistance: data.previousDistanceMeters,
            newDistance: data.newDistanceMeters,
            previousTime: data.previousTravelTimeSeconds,
            newTime: data.newTravelTimeSeconds
          });
        }
      } else {
        setRerouteStatus({ type: 'info', message: "Route affected but still optimal." });
      }
      
      setRouteResult(data.newRouteResult);
      setComparisonResults(null); 
    } else {
      setRerouteStatus(null);
    }
  };

  const handleAddIncident = () => {
    setIncidentError(null);
    setRerouteStatus(null);
    if (incidentFrom === '' || incidentTo === '') {
      setIncidentError("Select both nodes for the incident.");
      return;
    }

    const body = {
      from: parseInt(incidentFrom),
      to: parseInt(incidentTo),
      type: incidentType
    };

    if (routeResult && routeResult.found) {
      body.activeRoute = routeResult;
    }

    fetch(`${API_URL}/incident`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    })
    .then(async res => {
      const data = await res.json();
      if (!res.ok) throw new Error(data.error);
      
      setIncidentFrom('');
      setIncidentTo('');
      fetchCityAndIncidents();
      
      processRerouteResponse(data, false, body.from, body.to, body.type);
    })
    .catch(err => setIncidentError(err.message));
  }

  const handleClearIncident = (from, to) => {
    setIncidentError(null);
    setRerouteStatus(null);

    const body = { from, to };
    if (routeResult && routeResult.found) {
      body.activeRoute = routeResult;
    }

    fetch(`${API_URL}/incident`, {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    })
    .then(async res => {
      const data = await res.json();
      if (!res.ok) throw new Error(data.error);
      
      fetchCityAndIncidents();
      processRerouteResponse(data, true, from, to, 'clear');
    })
    .catch(err => console.error(err.message));
  }


  return (
    <div style={{ maxWidth: '1000px', margin: '0 auto', padding: '20px', fontFamily: 'sans-serif', color: '#2c3e50' }}>
      
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>
        <h1 style={{ margin: 0 }}>Smart Mobility Simulator</h1>
        <div style={{ display: 'flex', alignItems: 'center', gap: '15px' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '5px' }}>
            <label style={{ fontWeight: 'bold' }}>City Mode:</label>
            <select 
              value={cityMode} 
              onChange={(e) => handleCityModeChange(e.target.value)}
              disabled={isLoadingMode}
              style={{ padding: '5px', borderRadius: '4px', border: '1px solid #ccc' }}
            >
              <option value="generated">Generated City</option>
              <option value="montreal">Montreal</option>
            </select>
          </div>
          <p style={{ margin: 0, fontWeight: 'bold' }}>Backend Status: {backendStatus}</p>
        </div>
      </div>

      <div style={{ display: 'flex', gap: '20px', marginTop: '20px', flexWrap: 'wrap' }}>
        
        {/* Left Control Panel */}
        <div style={{ flex: '1', minWidth: '300px', display: 'flex', flexDirection: 'column', gap: '20px' }}>
          
          <div style={{ padding: '15px', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6' }}>
            <h2 style={{ marginTop: 0, fontSize: '1.2rem' }}>Routing</h2>
            <p style={{ fontSize: '0.9rem', color: '#7f8c8d', marginBottom: '15px' }}>Click nodes on the map to set.</p>
            
            <div style={{ marginBottom: '10px', display: 'flex', justifyContent: 'space-between' }}>
              <label style={{ fontWeight: 'bold' }}>Start Node:</label>
              <span>{startNode !== null ? startNode : '...'}</span>
            </div>
            
            <div style={{ marginBottom: '15px', display: 'flex', justifyContent: 'space-between' }}>
              <label style={{ fontWeight: 'bold' }}>Destination Node:</label>
              <span>{destNode !== null ? destNode : '...'}</span>
            </div>

            <div style={{ marginBottom: '10px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <label style={{ fontWeight: 'bold' }}>Algorithm:</label>
              <select 
                value={algorithm} 
                onChange={(e) => {
                  const newAlgo = e.target.value;
                  setAlgorithm(newAlgo);
                }}
                style={{ padding: '5px', borderRadius: '4px', border: '1px solid #ccc' }}
              >
                <option value="dijkstra">Dijkstra</option>
                <option value="astar">A*</option>
              </select>
            </div>

            <div style={{ marginBottom: '15px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <label style={{ fontWeight: 'bold' }}>Routing Objective:</label>
              <select 
                value={objective} 
                onChange={(e) => {
                  const newObj = e.target.value;
                  setObjective(newObj);
                }}
                style={{ padding: '5px', borderRadius: '4px', border: '1px solid #ccc' }}
              >
                <option value="shortest">Shortest</option>
                <option value="fastest">Fastest</option>
              </select>
            </div>

            <div style={{ padding: '10px', backgroundColor: '#e8f4fd', borderRadius: '4px', marginBottom: '15px' }}>
              <div style={{ marginBottom: '5px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <label style={{ fontWeight: 'bold', color: '#2980b9' }}>Route Preference (MVP5):</label>
                <select 
                  value={selectedProfile} 
                  onChange={(e) => setSelectedProfile(e.target.value)}
                  style={{ padding: '5px', borderRadius: '4px', border: '1px solid #ccc' }}
                >
                  {profiles.map(p => (
                    <option key={p} value={p}>{p.split('_').map(w => w.charAt(0).toUpperCase() + w.slice(1)).join(' ')}</option>
                  ))}
                </select>
              </div>
              {profileWeights && (
                <div style={{ fontSize: '0.85rem', color: '#34495e', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '5px' }}>
                  <span>Time: {(profileWeights.timeWeight * 100).toFixed(0)}%</span>
                  <span>Dist: {(profileWeights.distanceWeight * 100).toFixed(0)}%</span>
                  <span>Traffic: {(profileWeights.trafficWeight * 100).toFixed(0)}%</span>
                  <span>Cost: {(profileWeights.costWeight * 100).toFixed(0)}%</span>
                </div>
              )}
              
              <button 
                onClick={handleRecommendRoute}
                style={{ width: '100%', marginTop: '10px', padding: '10px', backgroundColor: '#f39c12', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: 'pointer', fontWeight: 'bold' }}
              >
                Recommend Route
              </button>
              
              <button 
                onClick={handleCompareProfiles}
                disabled={isComparingProfiles}
                style={{ width: '100%', marginTop: '10px', padding: '10px', backgroundColor: '#e67e22', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: isComparingProfiles ? 'not-allowed' : 'pointer', fontWeight: 'bold' }}
              >
                {isComparingProfiles ? 'Comparing...' : 'Compare All Profiles'}
              </button>
            </div>
            
            <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
              <button 
                onClick={() => handleFindRoute()}
                style={{ width: '100%', padding: '10px', backgroundColor: '#3498db', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: 'pointer', fontWeight: 'bold' }}
              >
                Find Route (Standard)
              </button>
              
              <button 
                onClick={handleCompareAlgorithms}
                disabled={isComparing}
                style={{ width: '100%', padding: '10px', backgroundColor: '#9b59b6', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: isComparing ? 'not-allowed' : 'pointer', fontWeight: 'bold' }}
              >
                {isComparing ? 'Comparing...' : 'Compare Algorithms'}
              </button>
            </div>
          </div>

          <div style={{ padding: '15px', backgroundColor: '#fdf3f2', borderRadius: '8px', border: '1px solid #f5b7b1' }}>
            <h2 style={{ marginTop: 0, fontSize: '1.2rem', color: '#c0392b' }}>Incidents</h2>
            
            <div style={{ display: 'flex', gap: '10px', marginBottom: '10px' }}>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>From:</label>
                <input type="number" value={incidentFrom} onChange={(e) => setIncidentFrom(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>To:</label>
                <input type="number" value={incidentTo} onChange={(e) => setIncidentTo(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
            </div>

            <div style={{ marginBottom: '15px' }}>
              <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Type:</label>
              <select value={incidentType} onChange={(e) => setIncidentType(e.target.value)} style={{ width: '100%', padding: '5px' }}>
                <option value="closure">Closure</option>
                <option value="accident">Accident</option>
              </select>
            </div>

            {incidentError && (
              <p style={{ color: '#c0392b', margin: '5px 0', fontSize: '0.9rem' }}>{incidentError}</p>
            )}

            <button onClick={handleAddIncident} style={{ width: '100%', padding: '8px', backgroundColor: '#e74c3c', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold' }}>
              Add Incident
            </button>

            {incidents.length > 0 && (
              <div style={{ marginTop: '15px' }}>
                <h4 style={{ margin: '0 0 10px 0' }}>Active Incidents</h4>
                <ul style={{ paddingLeft: '20px', margin: 0, fontSize: '0.9rem' }}>
                  {incidents.map((inc, i) => (
                    <li key={i} style={{ marginBottom: '5px' }}>
                      {inc.from} ↔ {inc.to} — {inc.type === 'closure' ? 'Closure' : 'Accident'}
                      <button 
                        onClick={() => handleClearIncident(inc.from, inc.to)}
                        style={{ marginLeft: '10px', fontSize: '0.7rem', padding: '2px 5px', cursor: 'pointer' }}
                      >
                        Clear
                      </button>
                    </li>
                  ))}
                </ul>
              </div>
            )}
          </div>

          <div style={{ padding: '15px', backgroundColor: '#eef2f5', borderRadius: '8px', border: '1px solid #d4dde4' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
              <h2 style={{ margin: 0, fontSize: '1.2rem', color: '#2c3e50' }}>Simulation</h2>
              <button onClick={handleResetSimulation} style={{ fontSize: '0.8rem', padding: '4px 8px', backgroundColor: '#95a5a6', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Reset
              </button>
            </div>
            
            <div style={{ display: 'flex', gap: '10px', marginBottom: '10px' }}>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Origin:</label>
                <input type="number" value={simOrigin} onChange={(e) => setSimOrigin(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Dest:</label>
                <input type="number" value={simDest} onChange={(e) => setSimDest(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
            </div>
            <div style={{ marginBottom: '10px' }}>
              <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Preference:</label>
              <select value={simPreference} onChange={(e) => setSimPreference(e.target.value)} style={{ width: '100%', padding: '5px', boxSizing: 'border-box' }}>
                {profiles.map(p => (
                  <option key={p} value={p}>{p.split('_').map(w => w.charAt(0).toUpperCase() + w.slice(1)).join(' ')}</option>
                ))}
              </select>
            </div>

            {simError && (
              <p style={{ color: '#c0392b', margin: '5px 0', fontSize: '0.9rem' }}>{simError}</p>
            )}

            <button onClick={handleSpawnVehicle} style={{ width: '100%', padding: '8px', backgroundColor: '#2ecc71', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold', marginBottom: '10px' }}>
              Spawn Single Vehicle
            </button>

            <div style={{ display: 'flex', gap: '5px', marginBottom: '15px' }}>
              <button onClick={() => handleSpawnBatch(10)} style={{ flex: 1, padding: '5px', backgroundColor: '#27ae60', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Spawn 10
              </button>
              <button onClick={() => handleSpawnBatch(50)} style={{ flex: 1, padding: '5px', backgroundColor: '#27ae60', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Spawn 50
              </button>
              <button onClick={() => handleSpawnBatch(100)} style={{ flex: 1, padding: '5px', backgroundColor: '#27ae60', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Spawn 100
              </button>
            </div>

            <div style={{ fontSize: '0.9rem', marginBottom: '10px', fontWeight: 'bold' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '5px' }}>
                <span>Vehicles: {vehicles.length}</span>
                <span>Simulation Time: {simulationTime.toFixed(1)} s</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', color: '#7f8c8d' }}>
                <span>Moving: {vehicles.filter(v => v.state === 'moving').length}</span>
                <span>Arrived: {vehicles.filter(v => v.state === 'arrived').length}</span>
              </div>
              {Object.keys(simPreferencesStats).length > 0 && (
                <div style={{ marginTop: '5px', color: '#7f8c8d', fontSize: '0.8rem', display: 'flex', flexWrap: 'wrap', gap: '5px' }}>
                  {Object.entries(simPreferencesStats).map(([pref, count]) => (
                    <span key={pref} style={{ backgroundColor: '#ecf0f1', padding: '2px 5px', borderRadius: '3px' }}>
                      {pref}: {count}
                    </span>
                  ))}
                </div>
              )}
            </div>

            <div style={{ display: 'flex', gap: '10px', marginBottom: '15px' }}>
              <button onClick={() => handleStepSimulation(1.0)} style={{ flex: 1, padding: '5px', backgroundColor: '#34495e', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Step +1s
              </button>
              <button onClick={() => handleStepSimulation(5.0)} style={{ flex: 1, padding: '5px', backgroundColor: '#34495e', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
                Step +5s
              </button>
            </div>

            {vehicles.length > 0 && (
              <div style={{ maxHeight: '150px', overflowY: 'auto', borderTop: '1px solid #d4dde4', paddingTop: '10px' }}>
                {vehicles.map((v) => (
                  <div key={v.id} style={{ backgroundColor: 'white', padding: '8px', borderRadius: '4px', marginBottom: '8px', border: '1px solid #ddd', fontSize: '0.85rem' }}>
                    <div style={{ fontWeight: 'bold', marginBottom: '4px' }}>Vehicle {v.id} ({v.preference})</div>
                    <div>{v.origin} → {v.destination}</div>
                    <div>State: <span style={{ textTransform: 'capitalize', color: v.state === 'waiting' ? '#f39c12' : (v.state === 'moving' ? '#3498db' : '#27ae60') }}>{v.state}</span></div>
                    {v.state === 'moving' && <div>Progress: {(v.progress * 100).toFixed(0)}%</div>}
                  </div>
                ))}
              </div>
            )}
          </div>

          <div style={{ padding: '15px', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6', minHeight: '150px' }}>
            <h2 style={{ marginTop: 0, fontSize: '1.2rem' }}>Current Route</h2>
            
            {rerouteStatus && (
              <div style={{ 
                marginBottom: '10px', 
                padding: '10px', 
                borderRadius: '4px',
                backgroundColor: rerouteStatus.type === 'error' ? '#fdf3f2' : (rerouteStatus.type === 'info' ? '#e8f4fd' : '#eafaf1'),
                border: `1px solid ${rerouteStatus.type === 'error' ? '#f5b7b1' : (rerouteStatus.type === 'info' ? '#b5dcf9' : '#a3e4d7')}`,
                color: rerouteStatus.type === 'error' ? '#c0392b' : (rerouteStatus.type === 'info' ? '#2980b9' : '#1e8449')
              }}>
                <strong>{rerouteStatus.message}</strong>
                {rerouteStatus.newTime !== undefined && (
                  <div style={{ marginTop: '5px', fontSize: '0.9rem' }}>
                    Before: {rerouteStatus.previousDistance} m | {rerouteStatus.previousTime.toFixed(1)} s<br/>
                    After: {rerouteStatus.newDistance} m | {rerouteStatus.newTime.toFixed(1)} s
                  </div>
                )}
              </div>
            )}

            {routeError && (
              <div style={{ color: '#e74c3c', fontWeight: 'bold' }}>
                Error: {routeError}
              </div>
            )}

            {!routeError && !routeResult && (
              <p style={{ color: '#7f8c8d', fontStyle: 'italic' }}>No route calculated yet.</p>
            )}

            {recommendationResult && (
              <div style={{ marginTop: '15px' }}>
                <h3 style={{ color: '#27ae60' }}>Recommended Route</h3>
                <p style={{ fontStyle: 'italic', color: '#7f8c8d', margin: '5px 0' }}>{recommendationResult.explanation}</p>
                <div style={{ backgroundColor: '#eafaf1', padding: '10px', borderRadius: '4px', border: '1px solid #2ecc71', marginBottom: '15px' }}>
                  <p style={{ margin: '2px 0' }}><strong>Profile:</strong> {recommendationResult.profile}</p>
                  <p style={{ margin: '2px 0' }}><strong>Score:</strong> {recommendationResult.recommendedRoute.score.toFixed(3)}</p>
                  <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px', marginTop: '10px', fontSize: '0.9rem' }}>
                    <div>
                      <strong>Distance:</strong> {recommendationResult.recommendedRoute.distanceMeters.toFixed(1)} m<br/>
                      <span style={{ color: '#7f8c8d', fontSize: '0.8rem' }}>Value: {recommendationResult.recommendedRoute.normalizedValues.distance.toFixed(2)} | Wt: {recommendationResult.recommendedRoute.scoreBreakdown.distance.toFixed(3)}</span>
                    </div>
                    <div>
                      <strong>Travel Time:</strong> {recommendationResult.recommendedRoute.travelTimeSeconds.toFixed(1)} s<br/>
                      <span style={{ color: '#7f8c8d', fontSize: '0.8rem' }}>Value: {recommendationResult.recommendedRoute.normalizedValues.time.toFixed(2)} | Wt: {recommendationResult.recommendedRoute.scoreBreakdown.time.toFixed(3)}</span>
                    </div>
                    <div>
                      <strong>Congestion:</strong> {recommendationResult.recommendedRoute.averageCongestion.toFixed(2)}x<br/>
                      <span style={{ color: '#7f8c8d', fontSize: '0.8rem' }}>Value: {recommendationResult.recommendedRoute.normalizedValues.traffic.toFixed(2)} | Wt: {recommendationResult.recommendedRoute.scoreBreakdown.traffic.toFixed(3)}</span>
                    </div>
                    <div>
                      <strong>Est. Cost:</strong> ${recommendationResult.recommendedRoute.estimatedCost.toFixed(2)}<br/>
                      <span style={{ color: '#7f8c8d', fontSize: '0.8rem' }}>Value: {recommendationResult.recommendedRoute.normalizedValues.cost.toFixed(2)} | Wt: {recommendationResult.recommendedRoute.scoreBreakdown.cost.toFixed(3)}</span>
                    </div>
                  </div>
                </div>
                
                <h4>Other Candidates ({recommendationResult.candidates.length})</h4>
                {recommendationResult.candidates.map((cand, idx) => {
                  const isRecommended = cand.score === recommendationResult.recommendedRoute.score && cand.distanceMeters === recommendationResult.recommendedRoute.distanceMeters;
                  return (
                    <div 
                      key={idx} 
                      onClick={() => {
                        setRouteResult({
                          found: true,
                          nodeIds: cand.nodeIds,
                          totalDistanceMeters: cand.distanceMeters,
                          estimatedTravelTimeSeconds: cand.travelTimeSeconds,
                          algorithm: cand.sourceObjective,
                          objective: cand.sourceObjective
                        });
                      }}
                      style={{ 
                        backgroundColor: isRecommended ? '#d5f5e3' : '#fdfefe', 
                        padding: '8px', 
                        borderRadius: '4px', 
                        border: isRecommended ? '2px solid #2ecc71' : '1px solid #d5dbdb', 
                        marginBottom: '8px', 
                        fontSize: '0.9rem',
                        cursor: 'pointer'
                      }}
                    >
                      <div style={{ fontWeight: 'bold', color: '#2980b9', display: 'flex', justifyContent: 'space-between' }}>
                        <span>Source: {cand.sourceObjective}</span>
                        {isRecommended && <span style={{ color: '#27ae60' }}>★ Recommended</span>}
                      </div>
                      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '4px', marginTop: '4px' }}>
                        <span>Dist: {cand.distanceMeters.toFixed(1)}m</span>
                        <span>Time: {cand.travelTimeSeconds.toFixed(1)}s</span>
                        <span>Cong: {cand.averageCongestion.toFixed(2)}x</span>
                        <span><strong>Score: {cand.score.toFixed(3)}</strong></span>
                      </div>
                    </div>
                  );
                })}
              </div>
            )}

            {routeResult && routeResult.found && !recommendationResult && (
              <div>
                <p style={{ margin: '5px 0' }}>Algorithm: <strong>{routeResult.algorithm === 'astar' ? 'A*' : 'Dijkstra'}</strong></p>
                <p style={{ margin: '5px 0' }}>Objective: <strong>{routeResult.objective === 'fastest' ? 'Fastest' : 'Shortest'}</strong></p>
                <div style={{ margin: '15px 0', padding: '10px', backgroundColor: '#ecf0f1', borderRadius: '4px', overflowX: 'auto', whiteSpace: 'nowrap' }}>
                  <strong>Route:</strong><br/>
                  {routeResult.nodeIds.join(' -> ')}
                </div>
                <p style={{ margin: '5px 0' }}>Distance: <strong>{routeResult.totalDistanceMeters} m</strong></p>
                <p style={{ margin: '5px 0' }}>Estimated travel time: <strong>{routeResult.estimatedTravelTimeSeconds.toFixed(1)} s</strong></p>
                {routeResult.nodesExplored !== undefined && <p style={{ margin: '5px 0' }}>Nodes explored: <strong>{routeResult.nodesExplored}</strong></p>}
                {routeResult.runtimeMicroseconds !== undefined && <p style={{ margin: '5px 0' }}>Runtime: <strong>{routeResult.runtimeMicroseconds} &micro;s</strong></p>}
              </div>
            )}
            
            {routeResult && !routeResult.found && (
              <div style={{ color: '#c0392b', fontWeight: 'bold' }}>
                Route completely blocked. No path available.
              </div>
            )}

            {profileComparison && (
              <div style={{ marginTop: '15px', backgroundColor: '#fdfefe', padding: '10px', borderRadius: '4px', border: '1px solid #d5dbdb' }}>
                <h3 style={{ color: '#e67e22', marginTop: 0 }}>Profile Comparison</h3>
                <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                  {profileComparison.map((p, idx) => (
                    <div 
                      key={idx} 
                      onClick={() => {
                        setRouteResult({
                          found: true,
                          nodeIds: p.nodeIds,
                          totalDistanceMeters: p.distanceMeters,
                          estimatedTravelTimeSeconds: p.travelTimeSeconds,
                          algorithm: p.profile,
                          objective: p.profile
                        });
                      }}
                      style={{ padding: '8px', border: '1px solid #ecf0f1', borderRadius: '4px', cursor: 'pointer', fontSize: '0.9rem' }}
                    >
                      <div style={{ fontWeight: 'bold', textTransform: 'capitalize' }}>{p.profile.replace('_', ' ')}</div>
                      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '4px', marginTop: '4px', color: '#34495e' }}>
                        <span>Dist: {p.distanceMeters.toFixed(1)}m</span>
                        <span>Time: {p.travelTimeSeconds.toFixed(1)}s</span>
                        <span>Cong: {p.averageCongestion.toFixed(2)}x</span>
                        <span>Cost: ${p.estimatedCost.toFixed(2)}</span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            )}
          </div>
          
        </div>

        {/* Right Map Panel */}
        <div style={{ flex: '2', minWidth: '400px' }}>
          <h2 style={{ textAlign: 'center', marginTop: 0 }}>CITY VISUALIZATION</h2>
          {isLoadingMode ? (
            <div style={{ textAlign: 'center', padding: '50px' }}>Loading {cityMode} map data...</div>
          ) : cityData.nodes.length > 0 ? (
            cityData.coordinateSystem === 'geographic' ? (
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
          ) : (
            <p style={{ textAlign: 'center', color: '#7f8c8d' }}>Loading city data...</p>
          )}
        </div>

      </div>

      {/* Comparison Section */}
      {(comparisonResults || comparisonError) && (
        <div style={{ marginTop: '30px', borderTop: '2px solid #ecf0f1', paddingTop: '20px' }}>
          <h2 style={{ textAlign: 'center', marginBottom: '20px' }}>Algorithm Comparison</h2>
          
          <p style={{ textAlign: 'center', color: '#7f8c8d', fontStyle: 'italic', marginBottom: '20px' }}>
            Note: Runtime may vary between runs, especially on small graphs.
          </p>

          {comparisonError && (
            <div style={{ color: '#e74c3c', fontWeight: 'bold', textAlign: 'center', marginBottom: '20px' }}>
              {comparisonError}
            </div>
          )}

          {comparisonResults && (
            <div style={{ display: 'flex', gap: '20px', flexWrap: 'wrap' }}>
              {renderComparisonCard('Dijkstra', comparisonResults.dijkstra, true)}
              {renderComparisonCard('A*', comparisonResults.astar, false)}
            </div>
          )}
        </div>
      )}

    </div>
  )
}

export default App
