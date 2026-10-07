# Deploying to Railway

This project uses a monorepo with a C++ backend and a React/Vite frontend.
Railway runs them as **two separate services** from the same GitHub repository.

---

## Overview

| Service  | Root Directory | Dockerfile             | PORT assigned by |
| -------- | -------------- | ---------------------- | ---------------- |
| Backend  | `/`            | `/Dockerfile`          | Railway (`PORT`) |
| Frontend | `/frontend`    | `/frontend/Dockerfile` | Railway (`PORT`) |

Railway automatically injects a `PORT` environment variable into every
container. Both the backend and the Caddy frontend web server read it.

---

## Montreal downtown data

The backend image includes the 449 KiB downtown street extract at
`/app/data/downtown.osm.pbf`, configured by `OSM_PBF_PATH`. No runtime download,
volume, or map API key is required. See [data provenance and extraction](../data/montreal/README.md).

The app starts with a clean practice grid. Select **Montréal Downtown** to load
the included streets around Concordia, McGill, and Old Montreal. If the data is
unavailable, the server returns `503 MAP_UNAVAILABLE` and preserves the current
city. The UI displays the error without replacing the map.

Only the small downtown extract is tracked. Full-city PBF files and generated
binary caches remain excluded from Git and Docker contexts. Browser background
tiles come from OpenStreetMap and require internet access; routing uses the
bundled street network. MapLibre's worker is emitted as a separate Vite asset.

---

## Environment Variables

### Backend Service

| Variable          | Description                                                | Example value                                 |
| ----------------- | ---------------------------------------------------------- | --------------------------------------------- |
| `PORT`            | **Set by Railway automatically.** Port to listen on.       | `8080` (Railway assigns this)                 |
| `FRONTEND_ORIGIN` | Allowed CORS origin (your Railway frontend public URL).    | `https://smart-mobility-frontend.railway.app` |
| `OSM_PBF_PATH`    | Optional dataset override; downtown is included in Docker. | `/app/data/downtown.osm.pbf`                  |

> **CORS note:** If `FRONTEND_ORIGIN` is not set, the backend defaults to
> `http://localhost:5173` for local development.

### Frontend Service

| Variable       | Description                                              | Example value                                |
| -------------- | -------------------------------------------------------- | -------------------------------------------- |
| `PORT`         | **Set by Railway automatically.** Port Caddy listens on. | `3000` (Railway assigns this)                |
| `VITE_API_URL` | Full HTTPS URL of the backend Railway service.           | `https://smart-mobility-backend.railway.app` |

> **IMPORTANT:** `VITE_API_URL` is baked into the frontend bundle at build
> time by Vite. You must set it as a Railway environment variable **before**
> Railway builds the service. If you change the backend URL later, you must
> trigger a **redeploy** of the frontend.

---

## Step-by-Step Railway Setup

### 1. Create a Railway project

Go to [railway.app](https://railway.app) → **New Project** → **Deploy from
GitHub repo** → select `smart-mobility-simulator`.

---

### 2. Create the Backend service

Railway will auto-detect the root `Dockerfile`.

In the service settings:

- **Root Directory:** `/` (leave at default)
- **Dockerfile Path:** `Dockerfile`
- **Health Check Path:** `/health`

Add environment variable:

```
FRONTEND_ORIGIN = https://<your-frontend-domain>.railway.app
```

(Set this after you know the frontend domain; you can redeploy backend after.)

Note the backend's **public URL** (e.g. `https://smart-mobility-backend-xxx.railway.app`).

---

### 3. Create the Frontend service

In the same Railway project, click **New** → **GitHub repo** → same repo.

In the service settings:

- **Root Directory:** `/frontend`
- **Dockerfile Path:** `Dockerfile` _(relative to root directory, so `/frontend/Dockerfile`)_

Add environment variable:

```
VITE_API_URL = https://<your-backend-domain>.railway.app
```

Railway will **build and deploy** the frontend. Caddy serves `dist/` and
handles SPA routing (any unknown path returns `index.html`).

---

### 4. Verify

| URL                         | Expected result    |
| --------------------------- | ------------------ |
| `https://<backend>/health`  | `{"status":"ok"}`  |
| `https://<frontend>/`       | React app loads    |
| `https://<frontend>/health` | `OK` (Caddy route) |

---

## Local Docker Testing

Test both images locally before pushing:

```bash
# Backend (listens on localhost:8400 in generated city mode)
docker build -t smart-mobility-backend .
docker run -p 8400:8400 \
  -e FRONTEND_ORIGIN=http://localhost:5173 \
  smart-mobility-backend

# Frontend (inject the local backend URL at build time)
docker build -t smart-mobility-frontend \
  --build-arg VITE_API_URL=http://localhost:8400 \
  ./frontend
docker run -p 3000:3000 smart-mobility-frontend
```

Then open `http://localhost:3000` and verify the app connects to the backend.

---

## Architecture Diagram

```
                        Railway
        ┌───────────────────────────────────────────┐
        │                                           │
        │   ┌──────────────────┐                   │
Browser─┼──▶│  Frontend        │                   │
        │   │  Caddy + dist/   │                   │
        │   │  PORT from env   │                   │
        │   └────────┬─────────┘                   │
        │            │ HTTPS API calls               │
        │            ▼                              │
        │   ┌──────────────────┐                   │
        │   │  Backend         │                   │
        │   │  C++/httplib     │                   │
        │   │  PORT from env   │                   │
        │   │  CORS from env   │                   │
        │   └──────────────────┘                   │
        │                                           │
        └───────────────────────────────────────────┘
```
