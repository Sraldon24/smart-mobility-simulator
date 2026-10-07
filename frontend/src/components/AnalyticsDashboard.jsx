import React, { useState, useEffect } from 'react';

const AnalyticsDashboard = () => {
  const [metrics, setMetrics] = useState(null);
  const [history, setHistory] = useState([]);
  
  useEffect(() => {
    const fetchMetrics = async () => {
      try {
        const res = await fetch('http://localhost:8400/metrics?history=true');
        if (res.ok) {
          const data = await res.json();
          setMetrics(data);
          if (data.history) {
            setHistory(data.history);
          }
        }
      } catch (err) {
        console.error("Failed to fetch analytics metrics", err);
      }
    };
    
    fetchMetrics();
    const interval = setInterval(fetchMetrics, 1000);
    return () => clearInterval(interval);
  }, []);

  if (!metrics) return null;

  const { routing, simulation, traffic } = metrics;

  return (
    <div style={{ marginTop: '30px', padding: '20px', backgroundColor: '#f4f6f7', borderRadius: '8px', border: '1px solid #d5dbdb' }}>
      <h2 style={{ marginTop: 0, textAlign: 'center', color: '#2c3e50' }}>Analytics Dashboard</h2>
      <p style={{ textAlign: 'center', color: '#7f8c8d', fontSize: '0.9rem', marginBottom: '20px' }}>
        City Mode: <strong style={{ textTransform: 'capitalize' }}>{metrics.cityMode}</strong>
      </p>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(250px, 1fr))', gap: '20px' }}>
        
        {/* Simulation Cards */}
        <div style={{ backgroundColor: 'white', padding: '15px', borderRadius: '8px', boxShadow: '0 2px 4px rgba(0,0,0,0.05)' }}>
          <h3 style={{ marginTop: 0, color: '#34495e', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>Simulation</h3>
          <p><strong>Total Vehicles:</strong> {simulation.vehicleCount}</p>
          <p><strong>Moving:</strong> {simulation.moving}</p>
          <p><strong>Arrived:</strong> {simulation.arrived}</p>
          <p><strong>Simulation Time:</strong> {simulation.simulationTimeSeconds.toFixed(1)} s</p>
          <p><strong>Avg Trip Time:</strong> {simulation.averageTripTimeSeconds.toFixed(1)} s</p>
          <p><strong>Update Time:</strong> {simulation.averageSimulationUpdateMicroseconds.toFixed(0)} &micro;s</p>
        </div>

        {/* Traffic Cards */}
        <div style={{ backgroundColor: 'white', padding: '15px', borderRadius: '8px', boxShadow: '0 2px 4px rgba(0,0,0,0.05)' }}>
          <h3 style={{ marginTop: 0, color: '#e67e22', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>Traffic</h3>
          <p><strong>Congested Roads:</strong> {traffic.congestedRoadCount}</p>
          <p><strong>Avg Congestion:</strong> {traffic.averageCongestionFactor.toFixed(2)}x</p>
          <p><strong>Total Reroutes:</strong> {traffic.totalReroutes}</p>
          
          {history.length > 0 && (
            <div style={{ marginTop: '15px' }}>
              <strong style={{ fontSize: '0.85rem' }}>Active vs Arrived Vehicles (Last 30)</strong>
              <div style={{ display: 'flex', alignItems: 'flex-end', height: '50px', gap: '2px', marginTop: '5px' }}>
                {history.slice(-30).map((h, i) => {
                  const max = Math.max(1, ...history.map(x => x.simulation.vehicleCount));
                  const movingHeight = (h.simulation.moving / max) * 100;
                  const arrivedHeight = (h.simulation.arrived / max) * 100;
                  return (
                    <div key={i} style={{ flex: 1, display: 'flex', flexDirection: 'column', justifyContent: 'flex-end', height: '100%' }}>
                      <div style={{ backgroundColor: '#2ecc71', height: `${arrivedHeight}%` }} title="Arrived" />
                      <div style={{ backgroundColor: '#3498db', height: `${movingHeight}%` }} title="Moving" />
                    </div>
                  );
                })}
              </div>
            </div>
          )}
        </div>

        {/* Algorithm Comparison / Routing */}
        <div style={{ backgroundColor: 'white', padding: '15px', borderRadius: '8px', boxShadow: '0 2px 4px rgba(0,0,0,0.05)' }}>
          <h3 style={{ marginTop: 0, color: '#9b59b6', borderBottom: '2px solid #ecf0f1', paddingBottom: '10px' }}>Algorithm Comparison</h3>
          {routing.dijkstraNodes > 0 || routing.astarNodes > 0 ? (
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '10px', fontSize: '0.9rem' }}>
              <div>
                <h4 style={{ margin: '0 0 5px 0' }}>Dijkstra</h4>
                <div>Nodes: {routing.dijkstraNodes}</div>
                <div>Runtime: {routing.dijkstraRuntime} &micro;s</div>
                <div>Dist: {routing.dijkstraDistance.toFixed(0)}m</div>
              </div>
              <div>
                <h4 style={{ margin: '0 0 5px 0' }}>A*</h4>
                <div>Nodes: {routing.astarNodes}</div>
                <div>Runtime: {routing.astarRuntime} &micro;s</div>
                <div>Dist: {routing.astarDistance.toFixed(0)}m</div>
              </div>
            </div>
          ) : (
            <p style={{ fontStyle: 'italic', color: '#7f8c8d' }}>Run an algorithm comparison to see data here.</p>
          )}
          
          <div style={{ marginTop: '15px', borderTop: '1px solid #ecf0f1', paddingTop: '10px' }}>
            <h4 style={{ margin: '0 0 5px 0' }}>Last Routing (Explicit)</h4>
            {routing.lastAlgorithm !== "none" ? (
              <div style={{ fontSize: '0.9rem' }}>
                <div>Algo: <span style={{ textTransform: 'capitalize' }}>{routing.lastAlgorithm}</span></div>
                <div>Dist: {routing.lastRouteDistanceMeters.toFixed(1)}m | Time: {routing.lastRouteTimeSeconds.toFixed(1)}s</div>
                <div>Nodes Explored: {routing.nodesExplored}</div>
              </div>
            ) : (
              <span style={{ fontSize: '0.9rem', fontStyle: 'italic', color: '#bdc3c7' }}>No explicit route run yet.</span>
            )}
          </div>
        </div>

      </div>
    </div>
  );
};

export default AnalyticsDashboard;

