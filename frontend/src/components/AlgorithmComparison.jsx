import React from 'react';

export default function AlgorithmComparison({ comparisonResults, comparisonError }) {
  if (!comparisonResults && !comparisonError) return null;

  const renderComparisonCard = (title, res, isFirst) => {
    if (!res) return null;
    return (
      <div style={{ flex: 1, minWidth: '250px', backgroundColor: '#f8f9fa', padding: '15px', borderRadius: '8px', border: '1px solid #dee2e6' }}>
        <h3 style={{ marginTop: 0, color: '#2c3e50', borderBottom: '2px solid #bdc3c7', paddingBottom: '10px' }}>{title}</h3>
        {!res.found ? (
          <p style={{ color: '#c0392b', fontWeight: 'bold' }}>No route found.</p>
        ) : (
          <div>
            <p><strong>Distance:</strong> {res.totalDistanceMeters} m</p>
            <p><strong>Travel Time:</strong> {res.estimatedTravelTimeSeconds.toFixed(1)} s</p>
            <p><strong>Nodes explored:</strong> {res.nodesExplored}</p>
            <p><strong>Runtime:</strong> {res.runtimeMicroseconds} &micro;s</p>
            <p><strong>Route segments:</strong> {res.nodeIds.length > 0 ? res.nodeIds.length - 1 : 0}</p>
          </div>
        )}
      </div>
    );
  };

  return (
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
  );
}
