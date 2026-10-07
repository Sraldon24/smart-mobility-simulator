import { useCallback, useEffect, useState } from "react";
import { request } from "./api";

const initial = {
  city: { nodes: [], roads: [] },
  mode: "generated",
  status: "loading",
  busy: false,
  simulation: {},
  vehicles: [],
  incidents: [],
  error: "",
  route: null,
  comparison: null,
  recommendation: null,
  message: "",
};

export function useSimulator() {
  const [state, setState] = useState(initial);
  const update = useCallback(
    (patch) => setState((previous) => ({ ...previous, ...patch })),
    [],
  );
  const refreshCity = useCallback(async () => {
    const [city, incidents] = await Promise.all([
      request("/city"),
      request("/incidents"),
    ]);
    update({
      city,
      incidents: incidents.incidents || [],
      mode: city.coordinateSystem === "geographic" ? "montreal" : "generated",
      status: "online",
    });
    return city;
  }, [update]);
  const refreshSimulation = useCallback(
    async (signal) => {
      const [simulation, vehicles] = await Promise.all([
        request("/simulation", { signal }),
        request("/vehicles", { signal }),
      ]);
      update({ simulation, vehicles, status: "online" });
    },
    [update],
  );

  useEffect(() => {
    const controller = new AbortController();
    let timer;
    let cityLoaded = false;
    const poll = async () => {
      try {
        if (!cityLoaded) {
          await refreshCity();
          cityLoaded = true;
        }
        await refreshSimulation(controller.signal);
        setState((previous) => ({
          ...previous,
          error:
            previous.error === "Connection lost. Reconnecting to the simulator…"
              ? ""
              : previous.error,
        }));
      } catch (error) {
        if (!controller.signal.aborted) {
          console.error("Simulator connection failed:", error);
          update({
            status: "offline",
            error: "Connection lost. Reconnecting to the simulator…",
          });
        }
      }
      if (!controller.signal.aborted) timer = setTimeout(poll, 1500);
    };
    const startTimer = setTimeout(poll, 0);
    return () => {
      controller.abort();
      clearTimeout(startTimer);
      clearTimeout(timer);
    };
  }, [refreshCity, refreshSimulation, update]);

  const perform = async (operation) => {
    update({ busy: true, error: "", message: "" });
    try {
      return await operation();
    } catch (error) {
      console.error("Simulator action failed:", error);
      update({
        error: error.message || "Unable to complete this action. Please retry.",
      });
      return null;
    } finally {
      update({ busy: false });
    }
  };

  const changeMode = (mode) =>
    perform(async () => {
      await request("/mode", { method: "POST", body: { mode } });
      const city = await refreshCity();
      await refreshSimulation();
      update({ route: null, comparison: null, recommendation: null });
      return city;
    });

  const findRoute = (start, end, algorithm, objective) =>
    perform(async () => {
      const route = await request(
        `/route?${new URLSearchParams({ start, end, algorithm, objective })}`,
      );
      update({ route, comparison: null, recommendation: null });
      return route;
    });

  const compare = (start, end, objective) =>
    perform(async () => {
      const results = await Promise.all(
        ["dijkstra", "astar"].map((algorithm) =>
          request(
            `/route?${new URLSearchParams({ start, end, algorithm, objective })}`,
          ),
        ),
      );
      update({
        comparison: { dijkstra: results[0], astar: results[1] },
        route: results[1],
      });
    });

  const recommend = (start, end, profile) =>
    perform(async () => {
      const recommendation = await request(
        `/recommend-route?${new URLSearchParams({ start, end, profile })}`,
      );
      const candidate = recommendation.recommendedRoute;
      update({
        recommendation,
        comparison: null,
        route: {
          found: true,
          nodeIds: candidate.nodeIds,
          totalDistanceMeters: candidate.distanceMeters,
          estimatedTravelTimeSeconds: candidate.travelTimeSeconds,
          algorithm: "Recommended",
          objective: candidate.sourceObjective,
        },
      });
    });

  const incident = (method, body, selection) =>
    perform(async () => {
      const activeRoute = state.route?.found
        ? {
            ...selection,
            nodeIds: state.route.nodeIds,
            totalDistanceMeters: state.route.totalDistanceMeters,
            estimatedTravelTimeSeconds: state.route.estimatedTravelTimeSeconds,
          }
        : undefined;
      const result = await request("/incident", {
        method,
        body: { ...body, ...(activeRoute ? { activeRoute } : {}) },
      });
      await refreshCity();
      if (result.rerouted && result.newRouteResult)
        update({
          route: result.newRouteResult,
          message: "Your route has been updated for the new road conditions.",
        });
      return result;
    });

  const simulate = (path, body) =>
    perform(async () => {
      await request(`/simulation/${path}`, { method: "POST", body });
      await refreshSimulation();
    });

  return {
    ...state,
    update,
    changeMode,
    findRoute,
    compare,
    recommend,
    incident,
    simulate,
  };
}
