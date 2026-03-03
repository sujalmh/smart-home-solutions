import '../../models/device_command.dart';
import '../../models/switch_module.dart';
import '../../utils/id_utils.dart';
import '../api_client.dart';

class DeviceRepository {
  final ApiClient api;

  DeviceRepository({required this.api});

  Future<DeviceCommand> sendCommand(DeviceCommand command) async {
    final nServer = normalizeId(command.serverId);
    final nDev = normalizeId(command.devId);
    final response = await api.postJson('/api/devices/$nServer/command', {
      'serverID': nServer,
      'devID': nDev,
      'comp': command.comp,
      'mod': command.mod,
      'stat': command.stat,
      'val': command.val,
      'reqId': ?command.reqId,
    });
    return DeviceCommand(
      serverId: response['serverID'] as String,
      devId: response['devID'] as String,
      comp: response['comp'] as String,
      mod: (response['mod'] as num).toInt(),
      stat: (response['stat'] as num).toInt(),
      val: (response['val'] as num).toInt(),
      reqId: response['reqId']?.toString(),
    );
  }

  Future<List<SwitchModule>> requestStatus({
    required String serverId,
    required String devId,
    String? comp,
    bool refresh = false,
  }) async {
    final nServer = normalizeId(serverId);
    final nDev = normalizeId(devId);
    final response = await api.postList('/api/devices/$nServer/status', {
      'serverID': nServer,
      'devID': nDev,
      'refresh': refresh,
      'comp': ?comp,
    });
    return response
        .map((item) => SwitchModule.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<void> configureServer({
    required String serverId,
    String? password,
    String? ip,
  }) async {
    final nServer = normalizeId(serverId);
    await api.postJson('/api/devices/$nServer/config/server', {
      'server_id': nServer,
      'pwd': ?password,
      'ip': ?ip,
    });
  }

  Future<void> configureRemote({
    required String serverId,
    required String ssid,
    required String password,
  }) async {
    final nServer = normalizeId(serverId);
    await api.postJson('/api/devices/$nServer/config/remote', {
      'ssid': ssid,
      'pwd': password,
    });
  }

  Future<void> configureClient({
    required String serverId,
    required String clientId,
    required String password,
    required String ip,
  }) async {
    final nServer = normalizeId(serverId);
    final nClient = normalizeId(clientId);
    await api.postJson('/api/devices/$nServer/config/client', {
      'client_id': nClient,
      'pwd': password,
      'ip': ip,
    });
  }

  Future<void> registerDevice({required String serverId}) async {
    final nServer = normalizeId(serverId);
    await api.postJson('/api/devices/$nServer/register', {
      'server_id': nServer,
    });
  }

  Future<bool> verifyGateway({required String serverId}) async {
    final nServer = normalizeId(serverId);
    final response = await api.getJson('/api/devices/$nServer/gateway/status');
    return response['online'] == true;
  }

  Future<void> bindSlave({
    required String serverId,
    required String clientId,
  }) async {
    final nServer = normalizeId(serverId);
    final nClient = normalizeId(clientId);
    await api.postJson('/api/devices/$nServer/gateway/bind', {
      'client_id': nClient,
    });
  }

  Future<void> unbindSlave({
    required String serverId,
    required String clientId,
  }) async {
    final nServer = normalizeId(serverId);
    final nClient = normalizeId(clientId);
    await api.postJson('/api/devices/$nServer/gateway/unbind', {
      'client_id': nClient,
    });
  }

  Future<List<Map<String, dynamic>>> listSeenSlaves({
    required String serverId,
  }) async {
    final nServer = normalizeId(serverId);
    final response = await api.getList('/api/devices/$nServer/gateway/seen');
    return response
        .map((item) => Map<String, dynamic>.from(item as Map))
        .toList();
  }

  Future<void> sendRoomCommand({
    required String roomId,
    required String deviceId,
    required String comp,
    required int mod,
    required int stat,
    required int val,
  }) async {
    await api.postJson('/api/rooms/$roomId/devices/$deviceId/commands', {
      'comp': comp,
      'mod': mod,
      'stat': stat,
      'val': val,
      'source': 'api',
    });
  }
}
