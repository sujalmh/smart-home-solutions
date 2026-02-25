# Smart Home Refactor + Upgrade Tasks (Phased)

## Phase 1 - Domain + Schema Foundation

- [x] Define v2 domain contracts for `Home -> Room -> Device -> DeviceLog` and map legacy `Server/Client/SwitchModule` fields.
- [x] Add backend modules for `rooms`, `devices`, and `device_logs` in `backend/app/models`, `backend/app/schemas`, and `backend/app/api`.
- [ ] Add DB migrations (Alembic) for `rooms`, `devices`, `device_logs` with FK constraints, timestamps, soft-delete flags, and indexes.
- [x] Run backfill migration to seed default rooms and map existing clients/switch modules into stable `devices` records.
- [ ] Define analytics metrics/dimensions contract (`energy_kwh`, `on_time`, `toggle_count`, `avg_load`; `home_id`, `room_id`, `device_id`, `time_bucket`).
- [ ] Define AI intent + entity schema (`turn_on`, `turn_off`, `set_brightness`, `status_check`, `schedule_action`) and validation rules.

## Phase 2 - Backend Refactor + Core APIs

- [ ] Refactor backend into `controllers -> services -> repositories` for registration, status sync, and command execution.
- [ ] Separate transport layer (WebSocket gateway emit/receive) from business/device orchestration logic.
- [x] Add room-based APIs: `GET /homes/{home_id}/rooms`, `GET /rooms/{room_id}/devices`, `POST /rooms/{room_id}/devices/{device_id}/commands`.
- [ ] Add room-device request/response schemas (pagination, filtering, validation, consistent error codes) and mark legacy payloads deprecated.
- [ ] Extend `device_logs` ingestion to store all state/value events with UTC timestamp and source (`api`, `socket`, `ai`, `voice`).
- [ ] Build analytics APIs for overview KPIs, trend series, room comparison, and device drill-down.
- [ ] Implement chat command handler API/service that resolves intents/entities and dispatches validated actions via command services.

## Phase 3 - Frontend Refactor + Dashboard

- [x] Update Flutter data layer (`repositories`, `providers`) to consume room-scoped APIs and centralize ID normalization.
- [x] Update app layout to room-first navigation: room sidebar (desktop) / drawer (mobile) plus room-scoped device cards.
- [ ] Build analytics dashboard UI with charts (usage trend, room split, device leaderboard) and loading/empty/error states.
- [ ] Add date-range + room filters in dashboard and sync filter state to analytics query params.
- [ ] Add AI assistant UI component with chat panel, voice controls, action confirmations, and fallback messaging.

## Phase 4 - Intelligence + Voice + Safety

- [ ] Implement NLU pipeline (intent classification + entity extraction) with confidence thresholds and fallback behavior.
- [ ] Add command resolver for canonical `home/room/device` mapping and ambiguity clarification flow.
- [ ] Add multi-turn context/session memory (last room/device, follow-up resolution, expiry policy).
- [ ] Integrate STT (speech-to-text) input and TTS (text-to-speech) output using provider-abstracted adapters.
- [ ] Add safety controls: auth checks, high-risk action confirmations, rate limiting, and prompt/content sanitization.
- [ ] Persist AI audit logs (utterance, intent/entities, selected action, safety decision, result).

## Phase 5 - Performance + Rollout Hardening

- [ ] Implement aggregation jobs/materialized summaries for hourly/daily/weekly rollups and top-consuming rooms/devices.
- [ ] Add CSV export endpoint and dashboard action for filtered analytics datasets.
- [ ] Add parity/regression tests for legacy + new control flows before decommissioning legacy routes.
- [ ] Add analytics correctness tests (sparse logs, missing intervals, DST/timezone boundaries).
- [ ] Add end-to-end tests for AI text/voice commands, ambiguous prompts, denied actions, and offline fallback.
- [ ] Execute staged rollout plan (feature flags, migration checks, monitoring), then remove deprecated server-centric endpoints.
