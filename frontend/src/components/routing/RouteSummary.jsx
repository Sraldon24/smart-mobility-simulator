import React from 'react';
import { Route, Clock, Zap, Target } from 'lucide-react';

export default function RouteSummary({ routeResult, routeError, rerouteStatus }) {
  if (routeError) {
    return (
      <div className="card" style={{ borderColor: 'var(--danger)', backgroundColor: 'var(--danger-muted)' }}>
        <h3 style={{ color: 'var(--danger)' }}>Routing Error</h3>
        <p className="text-sm">{routeError}</p>
      </div>
    );
  }

  if (routeResult && !routeResult.found) {
    return (
      <div className="card" style={{ borderColor: 'var(--danger)' }}>
        <h3 style={{ color: 'var(--danger)' }}>Route Blocked</h3>
        <p className="text-sm">No valid path exists between these nodes.</p>
      </div>
    );
  }

  if (!routeResult) return null;

  return (
    <div className="card flex-col gap-sm">
      <div className="flex-row" style={{ justifyContent: 'space-between' }}>
        <h3>Route Summary</h3>
        <span className="text-mono" style={{ fontSize: '0.8rem', color: 'var(--accent)' }}>
          {routeResult.algorithm.toUpperCase()} • {routeResult.objective.toUpperCase()}
        </span>
      </div>

      {rerouteStatus && (
        <div style={{ 
          padding: '0.5rem', 
          borderRadius: '4px',
          backgroundColor: rerouteStatus.type === 'error' ? 'var(--danger-muted)' : 'var(--accent-muted)',
          border: `1px solid ${rerouteStatus.type === 'error' ? 'var(--danger)' : 'var(--accent)'}`,
          fontSize: '0.85rem'
        }}>
          <strong>{rerouteStatus.message}</strong>
          {rerouteStatus.newTime !== undefined && (
            <div className="text-muted" style={{ marginTop: '0.25rem' }}>
              Was: {rerouteStatus.previousDistance}m ({rerouteStatus.previousTime.toFixed(1)}s)<br/>
              Now: {rerouteStatus.newDistance}m ({rerouteStatus.newTime.toFixed(1)}s)
            </div>
          )}
        </div>
      )}

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.5rem', marginTop: '0.5rem' }}>
        <div className="flex-row gap-sm">
          <Route size={16} className="text-muted" />
          <span className="text-sm font-medium">{(routeResult.totalDistanceMeters / 1000).toFixed(2)} km</span>
        </div>
        <div className="flex-row gap-sm">
          <Clock size={16} className="text-muted" />
          <span className="text-sm font-medium">{(routeResult.estimatedTravelTimeSeconds / 60).toFixed(1)} min</span>
        </div>
        <div className="flex-row gap-sm">
          <Target size={16} className="text-muted" />
          <span className="text-sm">{routeResult.nodesExplored} nodes</span>
        </div>
        <div className="flex-row gap-sm">
          <Zap size={16} className="text-muted" />
          <span className="text-sm">{(routeResult.runtimeMicroseconds / 1000).toFixed(2)} ms</span>
        </div>
      </div>
    </div>
  );
}

