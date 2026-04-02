import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../config/app_config.dart';
import '../data/api_client.dart';
import '../data/socket_client.dart';
import '../data/repositories/auth_repository.dart';
import '../data/repositories/ai_repository.dart';
import '../data/repositories/client_repository.dart';
import '../data/repositories/device_repository.dart';
import '../data/repositories/room_repository.dart';
import '../data/repositories/server_repository.dart';
import 'auth_controller.dart';
import 'auth_state.dart';
import 'chat_controller.dart';
import 'switch_modules_controller.dart';
import '../models/home.dart';
import '../models/switch_module.dart';
import '../models/server.dart';
import '../models/client.dart';
import '../models/room.dart';
import '../models/room_device.dart';

// ─── Gateway Status Notifier ─────────────────────────────────────────────────

/// Maintains a live map of `{ rawServerId → online }` for all known masters.
///
/// Status is initialised by calling [refresh] (triggered from [HomeScreen] on
/// load / manual refresh) and stays up-to-date via `gateway_status_changed`
/// socket push events emitted by the backend whenever a gateway connects or
/// disconnects.
class GatewayStatusNotifier extends StateNotifier<Map<String, bool>> {
  GatewayStatusNotifier({
    required this.deviceRepository,
    required this.socketClient,
  }) : super({}) {
    _sub = socketClient.gatewayStatus.listen(_onSocketEvent);
  }

  final DeviceRepository deviceRepository;
  final SocketClient socketClient;
  late final StreamSubscription<Map<String, dynamic>> _sub;

  void _onSocketEvent(Map<String, dynamic> data) {
    final serverId = data['serverID'];
    if (serverId is! String || serverId.isEmpty) return;
    final online = data['online'] == true;
    state = {...state, serverId: online};
  }

  /// Fetch the current gateway status for [serverId] from the REST API.
  Future<void> refresh(String serverId) async {
    try {
      final online = await deviceRepository.verifyGateway(serverId: serverId);
      state = {...state, serverId: online};
    } catch (_) {
      // Keep existing state rather than flipping to false on a transient error.
    }
  }

  @override
  void dispose() {
    _sub.cancel();
    super.dispose();
  }
}

// ─── Slave Status Notifier ───────────────────────────────────────────────────

/// Maintains a live map of `{ clientId → online }` for all known slaves.
///
/// Stays up-to-date via `slave_status_changed` socket push events.
class SlaveStatusNotifier extends StateNotifier<Map<String, bool>> {
  SlaveStatusNotifier({required this.socketClient}) : super({}) {
    _sub = socketClient.slaveStatus.listen(_onSocketEvent);
  }

  final SocketClient socketClient;
  late final StreamSubscription<Map<String, dynamic>> _sub;

  void _onSocketEvent(Map<String, dynamic> data) {
    final clientId = data['clientID'];
    if (clientId is! String || clientId.isEmpty) return;
    final online = data['online'] == true;
    state = {...state, clientId: online};
  }

  /// Seed initial slave health from a list of clients (e.g. from /bound API).
  void seedFromClients(List<Client> clients) {
    final updated = Map<String, bool>.from(state);
    for (final c in clients) {
      // Only seed if we don't already have a live value
      if (!updated.containsKey(c.clientId) && c.online != null) {
        updated[c.clientId] = c.online!;
      }
    }
    state = updated;
  }

  @override
  void dispose() {
    _sub.cancel();
    super.dispose();
  }
}

// ─── Providers ───────────────────────────────────────────────────────────────

final appConfigProvider = Provider<AppConfig>((ref) {
  return kUseLocal ? AppConfig.local() : AppConfig.production();
});

final publicApiClientProvider = Provider<ApiClient>((ref) {
  final config = ref.watch(appConfigProvider);
  return ApiClient(baseUrl: config.baseUrl);
});

final authenticatedApiClientProvider = Provider<ApiClient>((ref) {
  final config = ref.watch(appConfigProvider);
  final token = ref.watch(
    authControllerProvider.select((state) => state.token),
  );
  final headers = <String, String>{
    if (token != null && token.isNotEmpty) 'Authorization': 'Bearer $token',
  };
  return ApiClient(baseUrl: config.baseUrl, defaultHeaders: headers);
});

final socketClientProvider = Provider<SocketClient>((ref) {
  final config = ref.watch(appConfigProvider);
  final client = SocketClient(url: config.socketUrl, path: config.socketPath);
  ref.onDispose(client.dispose);
  return client;
});

final socketStatusProvider = StreamProvider<Map<String, dynamic>>((ref) {
  return ref.watch(socketClientProvider).status;
});

final socketResponseProvider = StreamProvider<Map<String, dynamic>>((ref) {
  return ref.watch(socketClientProvider).responses;
});

final socketConnectionProvider = StreamProvider<SocketConnectionState>((ref) {
  return ref.watch(socketClientProvider).connectionState;
});

final authRepositoryProvider = Provider<AuthRepository>((ref) {
  return AuthRepository(api: ref.watch(publicApiClientProvider));
});

final aiRepositoryProvider = Provider<AIRepository>((ref) {
  return AIRepository(api: ref.watch(authenticatedApiClientProvider));
});

final authControllerProvider = StateNotifierProvider<AuthController, AuthState>(
  (ref) => AuthController(
    authRepository: ref.watch(authRepositoryProvider),
    socketClient: ref.watch(socketClientProvider),
  ),
);

final serverRepositoryProvider = Provider<ServerRepository>((ref) {
  return ServerRepository(api: ref.watch(authenticatedApiClientProvider));
});

final serversProvider = FutureProvider<List<Server>>((ref) async {
  return ref.watch(serverRepositoryProvider).listServers();
});

final clientRepositoryProvider = Provider<ClientRepository>((ref) {
  return ClientRepository(api: ref.watch(authenticatedApiClientProvider));
});

final clientsProvider = FutureProvider.family<List<Client>, String>((
  ref,
  serverId,
) async {
  return ref.watch(clientRepositoryProvider).listClients(serverId: serverId);
});

final deviceRepositoryProvider = Provider<DeviceRepository>((ref) {
  return DeviceRepository(api: ref.watch(authenticatedApiClientProvider));
});

final roomRepositoryProvider = Provider<RoomRepository>((ref) {
  return RoomRepository(api: ref.watch(authenticatedApiClientProvider));
});

final homesProvider = FutureProvider<List<Home>>((ref) async {
  return ref.watch(roomRepositoryProvider).listHomes();
});

final roomsProvider = FutureProvider.family<List<Room>, String>((
  ref,
  homeId,
) async {
  return ref.watch(roomRepositoryProvider).listRooms(homeId);
});

final roomDevicesProvider = FutureProvider.family<List<RoomDevice>, String>((
  ref,
  roomId,
) async {
  return ref.watch(roomRepositoryProvider).listRoomDevices(roomId);
});

final gatewayStatusNotifierProvider =
    StateNotifierProvider<GatewayStatusNotifier, Map<String, bool>>((ref) {
      return GatewayStatusNotifier(
        deviceRepository: ref.watch(deviceRepositoryProvider),
        socketClient: ref.watch(socketClientProvider),
      );
    });

/// Sync per-server gateway status derived from [gatewayStatusNotifierProvider].
/// Returns `null` while a status has not yet been fetched (shows "Checking").
final gatewayStatusProvider = Provider.family<bool?, String>((ref, serverId) {
  return ref.watch(gatewayStatusNotifierProvider)[serverId];
});

final slaveStatusNotifierProvider =
    StateNotifierProvider<SlaveStatusNotifier, Map<String, bool>>((ref) {
      return SlaveStatusNotifier(
        socketClient: ref.watch(socketClientProvider),
      );
    });

/// Per-slave online status derived from [slaveStatusNotifierProvider].
/// Returns `null` if no status is known yet.
final slaveStatusProvider = Provider.family<bool?, String>((ref, clientId) {
  return ref.watch(slaveStatusNotifierProvider)[clientId];
});

final switchModulesProvider =
    StateNotifierProvider.family<
      SwitchModulesController,
      AsyncValue<List<SwitchModule>>,
      String
    >(
      (ref, clientId) => SwitchModulesController(
        clientId: clientId,
        clientRepository: ref.watch(clientRepositoryProvider),
        deviceRepository: ref.watch(deviceRepositoryProvider),
      ),
    );

final chatControllerProvider = StateNotifierProvider<ChatController, ChatState>(
  (ref) => ChatController(aiRepository: ref.watch(aiRepositoryProvider)),
);
