import React from 'react';

export default function RouteInfo({ routeError, routeResult, rerouteStatus }) {
  return (
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
  );
}
