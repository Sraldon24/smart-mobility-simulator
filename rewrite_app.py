import re

with open('frontend/src/App.jsx', 'r') as f:
    content = f.read()

# 1. Add new imports at the top
imports = """import React, { useState, useEffect } from 'react'
import CityMap from './components/CityMap'
import RouteInfo from './components/RouteInfo'
import IncidentControls from './components/IncidentControls'
import AlgorithmComparison from './components/AlgorithmComparison'
import './App.css'
"""
content = re.sub(r'import React.*?\nimport CityMap.*?\nimport \'./App.css\'\n', imports, content, flags=re.DOTALL)

# 2. Update processRerouteResponse
old_process = r'const processRerouteResponse = \(data, isClear = false\) => \{.*?\n  \};'
new_process = """const processRerouteResponse = (data, isClear = false, from = null, to = null, type = null) => {
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
  };"""
content = re.sub(old_process, new_process, content, flags=re.DOTALL)

# 3. Update calls to processRerouteResponse
content = content.replace("processRerouteResponse(data);", "processRerouteResponse(data, false, body.from, body.to, body.type);")
content = content.replace("processRerouteResponse(data, true);", "processRerouteResponse(data, true, from, to, 'clear');")

# 4. Replace the render method
render_match = re.search(r'return \(\n    <div className="App".*?\);', content, flags=re.DOTALL)
if render_match:
    new_render = """return (
    <div className="App" style={{ padding: '20px', fontFamily: 'sans-serif', maxWidth: '1200px', margin: '0 auto' }}>
      <h1 style={{ textAlign: 'center', color: '#2c3e50', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>
        Smart Mobility Simulator
      </h1>

      <div style={{ display: 'flex', gap: '30px', marginTop: '20px', flexWrap: 'wrap' }}>
        
        {/* Left Control Panel */}
        <div style={{ flex: '1', minWidth: '300px', display: 'flex', flexDirection: 'column', gap: '20px' }}>
          
          {/* Routing Controls */}
          <div style={{ padding: '15px', backgroundColor: '#f8f9fa', borderRadius: '8px', border: '1px solid #dee2e6' }}>
            <h2 style={{ marginTop: 0, fontSize: '1.2rem' }}>Plan Route</h2>
            
            <div style={{ display: 'flex', gap: '10px', marginBottom: '10px' }}>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Start Node:</label>
                <input type="number" value={startNode} onChange={(e) => setStartNode(parseInt(e.target.value) || 0)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
              <div style={{ flex: 1 }}>
                <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Dest Node:</label>
                <input type="number" value={destNode} onChange={(e) => setDestNode(parseInt(e.target.value) || 0)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
              </div>
            </div>

            <div style={{ marginBottom: '10px' }}>
              <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Algorithm:</label>
              <select value={algorithm} onChange={(e) => setAlgorithm(e.target.value)} style={{ width: '100%', padding: '5px' }}>
                <option value="dijkstra">Dijkstra</option>
                <option value="astar">A* (A-Star)</option>
              </select>
            </div>

            <div style={{ marginBottom: '15px' }}>
              <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Objective:</label>
              <select value={objective} onChange={(e) => setObjective(e.target.value)} style={{ width: '100%', padding: '5px' }}>
                <option value="fastest">Fastest Time</option>
                <option value="shortest">Shortest Distance</option>
              </select>
            </div>

            <div style={{ display: 'flex', gap: '10px' }}>
              <button 
                onClick={handleCalculateRoute} 
                style={{ flex: 2, padding: '10px', backgroundColor: '#3498db', color: 'white', border: 'none', borderRadius: '4px', fontSize: '1rem', cursor: 'pointer', fontWeight: 'bold' }}
              >
                Find Route
              </button>
              <button 
                onClick={handleCompareAlgorithms} 
                disabled={isComparing}
                style={{ flex: 1, padding: '10px', backgroundColor: '#9b59b6', color: 'white', border: 'none', borderRadius: '4px', fontSize: '0.9rem', cursor: isComparing ? 'not-allowed' : 'pointer', fontWeight: 'bold' }}
              >
                {isComparing ? 'Comparing...' : 'Compare'}
              </button>
            </div>
          </div>

          <IncidentControls 
            incidentFrom={incidentFrom} setIncidentFrom={setIncidentFrom}
            incidentTo={incidentTo} setIncidentTo={setIncidentTo}
            incidentType={incidentType} setIncidentType={setIncidentType}
            incidentError={incidentError} setIncidentError={setIncidentError}
            handleAddIncident={handleAddIncident}
            incidents={incidents} handleClearIncident={handleClearIncident}
            roads={cityData.roads}
          />

          <RouteInfo 
            routeError={routeError} 
            routeResult={routeResult} 
            rerouteStatus={rerouteStatus} 
          />
          
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

      <AlgorithmComparison 
        comparisonResults={comparisonResults} 
        comparisonError={comparisonError} 
      />

    </div>
  );"""
    
    # We also need to remove renderComparisonCard from App.jsx as it's now in AlgorithmComparison.jsx
    content = re.sub(r'const renderComparisonCard =.*?\}\;\n\n', '', content, flags=re.DOTALL)
    
    content = content[:render_match.start()] + new_render + content[render_match.end():]

with open('frontend/src/App.jsx', 'w') as f:
    f.write(content)
