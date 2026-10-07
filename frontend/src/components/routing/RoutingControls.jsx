import React from 'react';
import { MapPin, Navigation, Compass, Activity, Play } from 'lucide-react';

export default function RoutingControls({
  startNode,
  setStartNode,
  destNode,
  setDestNode,
  algorithm,
  setAlgorithm,
  objective,
  setObjective,
  onFindRoute,
  onCompare
}) {
  return (
    <div className="flex-col gap-md">
      <h2>Routing</h2>
      
      <div className="flex-col gap-sm">
        <label className="text-secondary text-sm">Start Node</label>
        <div className="flex-row gap-sm">
          <MapPin size={18} className="text-muted" />
          <input 
            type="number" 
            value={startNode === null ? '' : startNode} 
            onChange={(e) => setStartNode(e.target.value === '' ? null : parseInt(e.target.value))}
            placeholder="Select on map"
            style={{ flex: 1 }}
          />
        </div>
      </div>

      <div className="flex-col gap-sm">
        <label className="text-secondary text-sm">Destination Node</label>
        <div className="flex-row gap-sm">
          <Navigation size={18} className="text-muted" />
          <input 
            type="number" 
            value={destNode === null ? '' : destNode} 
            onChange={(e) => setDestNode(e.target.value === '' ? null : parseInt(e.target.value))}
            placeholder="Select on map"
            style={{ flex: 1 }}
          />
        </div>
      </div>

      <div className="flex-col gap-sm" style={{ marginTop: '0.5rem' }}>
        <label className="text-secondary text-sm">Algorithm</label>
        <div className="segmented-control">
          <button 
            className={algorithm === 'dijkstra' ? 'active' : ''} 
            onClick={() => setAlgorithm('dijkstra')}
          >
            Dijkstra
          </button>
          <button 
            className={algorithm === 'astar' ? 'active' : ''} 
            onClick={() => setAlgorithm('astar')}
          >
            A*
          </button>
        </div>
      </div>

      <div className="flex-col gap-sm">
        <label className="text-secondary text-sm">Objective</label>
        <div className="segmented-control">
          <button 
            className={objective === 'fastest' ? 'active' : ''} 
            onClick={() => setObjective('fastest')}
          >
            Fastest
          </button>
          <button 
            className={objective === 'shortest' ? 'active' : ''} 
            onClick={() => setObjective('shortest')}
          >
            Shortest
          </button>
          <button 
            className={objective === 'least_traffic' ? 'active' : ''} 
            onClick={() => setObjective('least_traffic')}
          >
            Least Traffic
          </button>
        </div>
      </div>

      <div className="flex-row gap-sm" style={{ marginTop: '0.5rem' }}>
        <button 
          className="btn btn-primary" 
          onClick={onFindRoute}
          disabled={startNode === null || destNode === null}
          style={{ flex: 1 }}
        >
          <Play size={16} /> Find Route
        </button>
        <button 
          className="btn" 
          onClick={onCompare}
          disabled={startNode === null || destNode === null}
          title="Compare Dijkstra vs A*"
        >
          <Activity size={16} /> Compare
        </button>
      </div>
    </div>
  );
}

