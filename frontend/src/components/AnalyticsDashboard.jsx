import React, { useState, useEffect } from 'react';
import { Activity, Car, Route, Clock } from 'lucide-react';

export default function AnalyticsDashboard({ apiUrl }) {
  const [metrics, setMetrics] = useState(null);
  const [history, setHistory] = useState([]);
  
  useEffect(() => {
    const fetchMetrics = async () => {
      try {
        const res = await fetch(`${apiUrl}/metrics?history=true`);
        if (res.ok) {
          const data = await res.json();
          setMetrics(data);
          if (data.history) {
            setHistory(data.history);
          }
        }
      } catch (err) {
        // Silently ignore if backend is not up yet
      }
    };
    
    fetchMetrics();
    const interval = setInterval(fetchMetrics, 1000);
    return () => clearInterval(interval);
  }, [apiUrl]);

  if (!metrics) return null;

  const { routing, simulation, traffic } = metrics;

  return (
    <div className="panel flex-col gap-md">
      <div className="flex-row" style={{ justifyContent: 'space-between', borderBottom: '1px solid var(--border)', paddingBottom: '0.5rem' }}>
        <h2 style={{ margin: 0, borderBottom: 'none', padding: 0 }} className="flex-row gap-sm"><Activity size={18}/> Global Analytics</h2>
        <span className="text-secondary text-sm">City Mode: <strong style={{ textTransform: 'capitalize', color: 'var(--text-primary)' }}>{metrics.cityMode}</strong></span>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))', gap: '1rem' }}>
        
        {/* Simulation */}
        <div className="card flex-col gap-sm">
          <h3 className="flex-row gap-sm"><Car size={16}/> Simulation</h3>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.5rem', fontSize: '0.85rem' }}>
            <div className="flex-col"><span className="text-muted">Total Vehicles</span><span className="font-bold text-mono">{simulation.vehicleCount}</span></div>
            <div className="flex-col"><span className="text-muted">Moving</span><span className="font-bold text-mono text-accent" style={{color: 'var(--accent)'}}>{simulation.moving}</span></div>
            <div className="flex-col"><span className="text-muted">Arrived</span><span className="font-bold text-mono text-success" style={{color: 'var(--success)'}}>{simulation.arrived}</span></div>
            <div className="flex-col"><span className="text-muted">Avg Trip Time</span><span className="font-bold text-mono">{simulation.averageTripTimeSeconds.toFixed(1)}s</span></div>
            <div className="flex-col"><span className="text-muted">Update Loop</span><span className="font-bold text-mono">{simulation.averageSimulationUpdateMicroseconds.toFixed(0)}µs</span></div>
            <div className="flex-col"><span className="text-muted">Sim Time</span><span className="font-bold text-mono">{simulation.simulationTimeSeconds.toFixed(1)}s</span></div>
          </div>
        </div>

        {/* Traffic */}
        <div className="card flex-col gap-sm">
          <h3 className="flex-row gap-sm"><Activity size={16} color="var(--warning)"/> Traffic</h3>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.5rem', fontSize: '0.85rem' }}>
            <div className="flex-col"><span className="text-muted">Congested Roads</span><span className="font-bold text-mono">{traffic.congestedRoadCount}</span></div>
            <div className="flex-col"><span className="text-muted">Avg Congestion</span><span className="font-bold text-mono">{traffic.averageCongestionFactor.toFixed(2)}x</span></div>
            <div className="flex-col"><span className="text-muted">Total Reroutes</span><span className="font-bold text-mono">{traffic.totalReroutes}</span></div>
          </div>
          
          {history.length > 0 && (
            <div className="flex-col" style={{ marginTop: '0.5rem', height: '40px', justifyContent: 'flex-end' }}>
              <div style={{ display: 'flex', alignItems: 'flex-end', height: '100%', gap: '1px' }}>
                {history.slice(-30).map((h, i) => {
                  const max = Math.max(1, ...history.map(x => x.simulation.vehicleCount));
                  const movingHeight = (h.simulation.moving / max) * 100;
                  const arrivedHeight = (h.simulation.arrived / max) * 100;
                  return (
                    <div key={i} style={{ flex: 1, display: 'flex', flexDirection: 'column', justifyContent: 'flex-end', height: '100%' }}>
                      <div style={{ backgroundColor: 'var(--success)', height: `${arrivedHeight}%` }} />
                      <div style={{ backgroundColor: 'var(--accent)', height: `${movingHeight}%` }} />
                    </div>
                  );
                })}
              </div>
              <span className="text-muted" style={{ fontSize: '0.6rem', textAlign: 'center', marginTop: '2px' }}>Vehicles: Moving (Blue) / Arrived (Green)</span>
            </div>
          )}
        </div>

        {/* Routing */}
        <div className="card flex-col gap-sm">
          <h3 className="flex-row gap-sm"><Route size={16}/> Routing</h3>
          {routing.dijkstraRequests > 0 || routing.astarRequests > 0 ? (
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.5rem', fontSize: '0.85rem' }}>
              <div className="flex-col"><span className="text-muted">Dijkstra Req</span><span className="font-bold text-mono">{routing.dijkstraRequests}</span></div>
              <div className="flex-col"><span className="text-muted">A* Req</span><span className="font-bold text-mono">{routing.astarRequests}</span></div>
              <div className="flex-col"><span className="text-muted">Dijkstra Runtime</span><span className="font-bold text-mono">{routing.averageDijkstraRuntimeMicroseconds.toFixed(0)}µs</span></div>
              <div className="flex-col"><span className="text-muted">A* Runtime</span><span className="font-bold text-mono">{routing.averageAstarRuntimeMicroseconds.toFixed(0)}µs</span></div>
              <div className="flex-col"><span className="text-muted">Dijkstra Explored</span><span className="font-bold text-mono">{routing.averageDijkstraNodesExplored.toFixed(0)}</span></div>
              <div className="flex-col"><span className="text-muted">A* Explored</span><span className="font-bold text-mono">{routing.averageAstarNodesExplored.toFixed(0)}</span></div>
            </div>
          ) : (
            <span className="text-muted text-sm" style={{ fontStyle: 'italic' }}>No routes calculated yet.</span>
          )}
        </div>

      </div>
    </div>
  );
}
