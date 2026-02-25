import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../config/app_config.dart';
import '../data/api_client.dart';
import '../data/socket_client.dart';
import '../data/repositories/auth_repository.dart';
import '../data/repositories/client_repository.dart';
import '../data/repositories/device_repository.dart';
import '../data/repositories/room_repository.dart';
import '../data/repositories/server_repository.dart';
import 'auth_controller.dart';
import 'auth_state.dart';
import 'switch_modules_controller.dart';
import '../models/home.dart';
import '../models/switch_module.dart';
import '../models/server.dart';
import '../models/client.dart';
import '../models/room.dart';
import '../models/room_device.dart';

final appConfigProvider = Provider<AppConfig>((ref) {
  return AppConfig.production();
});

final apiClientProvider = Provider<ApiClient>((ref) {
  final config = ref.watch(appConfigProvider);
  return ApiClient(baseUrl: config.baseUrl);
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

final authRepositoryProvider = Provider<AuthRepository>((ref) {
  return AuthRepository(api: ref.watch(apiClientProvider));
});

final authControllerProvider = StateNotifierProvider<AuthController, AuthState>(
  (ref) => AuthController(
    authRepository: ref.watch(authRepositoryProvider),
    socketClient: ref.watch(socketClientProvider),
  ),
);

final serverRepositoryProvider = Provider<ServerRepository>((ref) {
  return ServerRepository(api: ref.watch(apiClientProvider));
});

final serversProvider = FutureProvider<List<Server>>((ref) async {
  return ref.watch(serverRepositoryProvider).listServers();
});

final clientRepositoryProvider = Provider<ClientRepository>((ref) {
  return ClientRepository(api: ref.watch(apiClientProvider));
});

final clientsProvider = FutureProvider.family<List<Client>, String>((
  ref,
  serverId,
) async {
  return ref.watch(clientRepositoryProvider).listClients(serverId: serverId);
});

final deviceRepositoryProvider = Provider<DeviceRepository>((ref) {
  return DeviceRepository(api: ref.watch(apiClientProvider));
});

final roomRepositoryProvider = Provider<RoomRepository>((ref) {
  return RoomRepository(api: ref.watch(apiClientProvider));
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

final gatewayStatusProvider = FutureProvider.family<bool, String>((
  ref,
  serverId,
) async {
  return ref.watch(deviceRepositoryProvider).verifyGateway(serverId: serverId);
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
