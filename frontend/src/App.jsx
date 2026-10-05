import { useState, useEffect } from 'react'
import CityMap from './components/CityMap'

function App() {
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

  const API_URL = 'http://localhost:8080'

  const fetchCityAndIncidents = () => {
    fetch(`${API_URL}/city`)
      .then(res => res.json())
      .then(data => setCityData(data))
      .catch(err => console.error("Failed to load city:", err))

    fetch(`${API_URL}/incidents`)
      .then(res => res.json())
      .then(data => setIncidents(data.incidents || []))
      .catch(err => console.error("Failed to load incidents:", err))
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
  }, [])

  const [algorithm, setAlgorithm] = useState('dijkstra')
  const [objective, setObjective] = useState('shortest')

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
        <p style={{ margin: 0, fontWeight: 'bold' }}>Backend Status: {backendStatus}</p>
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
              <label style={{ fontWeight: 'bold' }}>Route Preference:</label>
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
            
            <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
              <button 
                onClick={() => handleFindRoute()}
                style={{ width: '100%', padding: '10px', backgroundColor: '#3498db', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: 'pointer', fontWeight: 'bold' }}
              >
                Find Route
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

            {routeResult && routeResult.found && (
              <div>
                <p style={{ margin: '5px 0' }}>Algorithm: <strong>{routeResult.algorithm === 'astar' ? 'A*' : 'Dijkstra'}</strong></p>
                <p style={{ margin: '5px 0' }}>Objective: <strong>{routeResult.objective === 'fastest' ? 'Fastest' : 'Shortest'}</strong></p>
                <div style={{ margin: '15px 0', padding: '10px', backgroundColor: '#ecf0f1', borderRadius: '4px', overflowX: 'auto', whiteSpace: 'nowrap' }}>
                  <strong>Route:</strong><br/>
                  {routeResult.nodeIds.join(' -> ')}
                </div>
                <p style={{ margin: '5px 0' }}>Distance: <strong>{routeResult.totalDistanceMeters} m</strong></p>
                <p style={{ margin: '5px 0' }}>Estimated travel time: <strong>{routeResult.estimatedTravelTimeSeconds.toFixed(1)} s</strong></p>
                <p style={{ margin: '5px 0' }}>Nodes explored: <strong>{routeResult.nodesExplored}</strong></p>
                <p style={{ margin: '5px 0' }}>Runtime: <strong>{routeResult.runtimeMicroseconds} &micro;s</strong></p>
              </div>
            )}
            
            {routeResult && !routeResult.found && (
              <div style={{ color: '#c0392b', fontWeight: 'bold' }}>
                Route completely blocked. No path available.
              </div>
            )}
          </div>
          
        </div>

        {/* Right Map Panel */}
        <div style={{ flex: '2', minWidth: '400px' }}>
          <h2 style={{ textAlign: 'center', marginTop: 0 }}>CITY VISUALIZATION</h2>
          {cityData.nodes.length > 0 ? (
            <CityMap 
              nodes={cityData.nodes} 
              roads={cityData.roads} 
              incidents={incidents}
              startNode={startNode} 
              destNode={destNode}
              routeNodeIds={routeResult?.found ? routeResult.nodeIds : []}
              onNodeClick={handleNodeClick}
            />
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
