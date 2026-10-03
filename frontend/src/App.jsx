import { useState, useEffect } from 'react'
import CityMap from './components/CityMap'

function App() {
  const [backendStatus, setBackendStatus] = useState('Checking...')
  const [cityData, setCityData] = useState({ nodes: [], roads: [] })
  
  const [startNode, setStartNode] = useState(0)
  const [destNode, setDestNode] = useState(24)
  
  const [routeResult, setRouteResult] = useState(null)
  const [routeError, setRouteError] = useState(null)

  const [comparisonResults, setComparisonResults] = useState(null)
  const [isComparing, setIsComparing] = useState(false)
  const [comparisonError, setComparisonError] = useState(null)
  
  const API_URL = 'http://localhost:8080'

  useEffect(() => {
    fetch(`${API_URL}/health`)
      .then(res => res.json())
      .then(data => {
        if (data.status === 'ok') setBackendStatus('Connected 🟢')
        else setBackendStatus('Error 🔴')
      })
      .catch(() => setBackendStatus('Disconnected 🔴'))

    fetch(`${API_URL}/city`)
      .then(res => res.json())
      .then(data => setCityData(data))
      .catch(err => console.error("Failed to load city:", err))
  }, [])

  const [algorithm, setAlgorithm] = useState('dijkstra')

  const handleFindRoute = (overrideAlgo = algorithm) => {
    if (startNode === null || destNode === null) {
      setRouteError("Please select both a Start and Destination node.");
      return;
    }

    setRouteResult(null)
    setRouteError(null)
    setComparisonResults(null)
    setComparisonError(null)

    fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=${overrideAlgo}`)
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

    try {
      const pDijkstra = fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=dijkstra`)
        .then(res => res.json().then(data => ({ res, data })))
        .catch(err => ({ error: err.message }));

      const pAStar = fetch(`${API_URL}/route?start=${startNode}&end=${destNode}&algorithm=astar`)
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
    } else if (startNode === null) {
      setStartNode(id);
    } else {
      setDestNode(id);
    }
  };

  const renderComparisonCard = (title, data, isDijkstra) => {
    const isSelected = routeResult && routeResult.algorithm === (isDijkstra ? 'dijkstra' : 'astar');
    
    return (
      <div style={{ 
        flex: 1, 
        padding: '15px', 
        backgroundColor: isSelected ? '#e8f4fd' : '#f8f9fa', 
        borderRadius: '8px', 
        border: isSelected ? '2px solid #3498db' : '1px solid #dee2e6' 
      }}>
        <h3 style={{ marginTop: 0, borderBottom: '1px solid #ccc', paddingBottom: '5px' }}>{title}</h3>
        
        {data.failed ? (
          <p style={{ color: '#e74c3c' }}>Error: {data.error}</p>
        ) : !data.found ? (
          <p style={{ color: '#e74c3c' }}>No route found.</p>
        ) : (
          <div>
            <p style={{ margin: '5px 0' }}>Distance: <strong>{data.totalDistanceMeters} m</strong></p>
            <p style={{ margin: '5px 0' }}>Nodes explored: <strong>{data.nodesExplored}</strong></p>
            <p style={{ margin: '5px 0' }}>Runtime: <strong>{data.runtimeMicroseconds} &micro;s</strong></p>
            <p style={{ margin: '5px 0' }}>Route segments: <strong>{data.nodeIds.length - 1}</strong></p>
            
            <button 
              onClick={() => setRouteResult(data)}
              style={{ 
                marginTop: '15px', 
                width: '100%', 
                padding: '8px', 
                backgroundColor: isSelected ? '#2ecc71' : '#bdc3c7', 
                color: 'white', 
                border: 'none', 
                borderRadius: '4px', 
                cursor: 'pointer',
                fontWeight: 'bold'
              }}
            >
              {isSelected ? 'Currently Showing' : `Show ${title} Route`}
            </button>
          </div>
        )}
      </div>
    );
  };

  return (
    <div style={{ maxWidth: '900px', margin: '0 auto', padding: '20px', fontFamily: 'sans-serif', color: '#2c3e50' }}>
      
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>
        <h1 style={{ margin: 0 }}>Smart Mobility Simulator</h1>
        <p style={{ margin: 0, fontWeight: 'bold' }}>Backend Status: {backendStatus}</p>
      </div>

      <div style={{ display: 'flex', gap: '20px', marginTop: '20px', flexWrap: 'wrap' }}>
        
        {/* Left Control Panel */}
        <div style={{ flex: '1', minWidth: '250px', display: 'flex', flexDirection: 'column', gap: '20px' }}>
          
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

            <div style={{ marginBottom: '15px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <label style={{ fontWeight: 'bold' }}>Algorithm:</label>
              <select 
                value={algorithm} 
                onChange={(e) => {
                  const newAlgo = e.target.value;
                  setAlgorithm(newAlgo);
                  if (startNode !== null && destNode !== null && !comparisonResults) {
                    handleFindRoute(newAlgo);
                  }
                }}
                style={{ padding: '5px', borderRadius: '4px', border: '1px solid #ccc' }}
              >
                <option value="dijkstra">Dijkstra</option>
                <option value="astar">A*</option>
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

          <div style={{ padding: '15px', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6', minHeight: '150px' }}>
            <h2 style={{ marginTop: 0, fontSize: '1.2rem' }}>Current Route</h2>
            
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
                <div style={{ margin: '15px 0', padding: '10px', backgroundColor: '#ecf0f1', borderRadius: '4px', overflowX: 'auto', whiteSpace: 'nowrap' }}>
                  <strong>Route:</strong><br/>
                  {routeResult.nodeIds.join(' -> ')}
                </div>
                <p style={{ margin: '5px 0' }}>Distance: <strong>{routeResult.totalDistanceMeters} m</strong></p>
                <p style={{ margin: '5px 0' }}>Nodes explored: <strong>{routeResult.nodesExplored}</strong></p>
                <p style={{ margin: '5px 0' }}>Runtime: <strong>{routeResult.runtimeMicroseconds} &micro;s</strong></p>
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
