import React from 'react';
import { Activity, Clock, Zap, Map } from 'lucide-react';

export default function AlgorithmComparison({ comparisonResults, comparisonError }) {
  if (!comparisonResults && !comparisonError) return null;

  const renderComparisonCard = (title, res, isWinner) => {
    if (!res) return null;
    return (
      <div className={`panel comparison-card ${isWinner ? 'winner' : ''}`}>
        <div className="panel-header">
          <h3>{title}</h3>
          {isWinner && <span className="winner-badge"><Zap size={14} /> Fastest</span>}
        </div>
        
        {!res.found ? (
          <div className="error-message">No route found.</div>
        ) : (
          <div className="metrics-grid">
            <div className="metric-item">
              <span className="metric-label"><Map size={14} /> Distance</span>
              <span className="metric-value">{res.totalDistanceMeters} m</span>
            </div>
            <div className="metric-item">
              <span className="metric-label"><Clock size={14} /> Travel Time</span>
              <span className="metric-value">{res.estimatedTravelTimeSeconds.toFixed(1)} s</span>
            </div>
            <div className="metric-item">
              <span className="metric-label"><Activity size={14} /> Nodes Explored</span>
              <span className="metric-value highlight">{res.nodesExplored}</span>
            </div>
            <div className="metric-item">
              <span className="metric-label"><Zap size={14} /> Runtime</span>
              <span className="metric-value highlight">{res.runtimeMicroseconds} &micro;s</span>
            </div>
          </div>
        )}
      </div>
    );
  };

  const dTime = comparisonResults?.dijkstra?.runtimeMicroseconds || Infinity;
  const aTime = comparisonResults?.astar?.runtimeMicroseconds || Infinity;

  return (
    <div className="panel algorithm-comparison-panel">
      <div className="panel-header">
        <h2>Algorithm Performance Comparison</h2>
      </div>
      
      {comparisonError && (
        <div className="error-message">
          {comparisonError}
        </div>
      )}

      {comparisonResults && (
        <div className="comparison-cards">
          {renderComparisonCard('Dijkstra', comparisonResults.dijkstra, dTime <= aTime)}
          {renderComparisonCard('A*', comparisonResults.astar, aTime < dTime)}
        </div>
      )}
    </div>
  );
}
