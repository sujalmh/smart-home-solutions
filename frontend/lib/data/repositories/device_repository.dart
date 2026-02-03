import '../../models/device_command.dart';
import '../../models/switch_module.dart';
import '../api_client.dart';

class DeviceRepository {
  final ApiClient api;

  DeviceRepository({required this.api});

  String _normalizeId(String value) {
    if (value.startsWith('RSW-')) {
      return value;
    }
    return 'RSW-$value';
  }

  Future<DeviceCommand> sendCommand(DeviceCommand command) async {
    final normalizedServer = _normalizeId(command.serverId);
    final normalizedDev = _normalizeId(command.devId);
    final response = await api
        .postJson('/api/devices/$normalizedServer/command', {
          'serverID': normalizedServer,
          'devID': normalizedDev,
          'comp': command.comp,
          'mod': command.mod,
          'stat': command.stat,
          'val': command.val,
        });
    return DeviceCommand(
      serverId: response['serverID'] as String,
      devId: response['devID'] as String,
      comp: response['comp'] as String,
      mod: (response['mod'] as num).toInt(),
      stat: (response['stat'] as num).toInt(),
      val: (response['val'] as num).toInt(),
    );
  }

  Future<List<SwitchModule>> requestStatus({
    required String serverId,
    required String devId,
    String? comp,
  }) async {
    final normalizedServer = _normalizeId(serverId);
    final normalizedDev = _normalizeId(devId);
    final response = await api
        .postList('/api/devices/$normalizedServer/status', {
          'serverID': normalizedServer,
          'devID': normalizedDev,
          if (comp != null) 'comp': comp,
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
    final normalizedServer = _normalizeId(serverId);
    await api.postJson('/api/devices/$normalizedServer/config/server', {
      'server_id': normalizedServer,
      if (password != null) 'pwd': password,
      if (ip != null) 'ip': ip,
    });
  }

  Future<void> configureRemote({
    required String serverId,
    required String ssid,
    required String password,
  }) async {
    final normalizedServer = _normalizeId(serverId);
    await api.postJson('/api/devices/$normalizedServer/config/remote', {
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
    final normalizedServer = _normalizeId(serverId);
    final normalizedClient = _normalizeId(clientId);
    await api.postJson('/api/devices/$normalizedServer/config/client', {
      'client_id': normalizedClient,
      'pwd': password,
      'ip': ip,
    });
  }

  Future<void> registerDevice({required String serverId}) async {
    final normalizedServer = _normalizeId(serverId);
    await api.postJson('/api/devices/$normalizedServer/register', {
      'server_id': normalizedServer,
    });
  }

  Future<bool> verifyGateway({required String serverId}) async {
    final normalizedServer = _normalizeId(serverId);
    final response = await api.getJson(
      '/api/devices/$normalizedServer/gateway/status',
    );
    return response['online'] == true;
  }

  Future<void> bindSlave({
    required String serverId,
    required String clientId,
  }) async {
    final normalizedServer = _normalizeId(serverId);
    final normalizedClient = _normalizeId(clientId);
    await api.postJson('/api/devices/$normalizedServer/gateway/bind', {
      'client_id': normalizedClient,
    });
  }

  Future<void> unbindSlave({
    required String serverId,
    required String clientId,
  }) async {
    final normalizedServer = _normalizeId(serverId);
    final normalizedClient = _normalizeId(clientId);
    await api.postJson('/api/devices/$normalizedServer/gateway/unbind', {
      'client_id': normalizedClient,
    });
  }

  Future<List<Map<String, dynamic>>> listSeenSlaves({
    required String serverId,
  }) async {
    final normalizedServer = _normalizeId(serverId);
    final response = await api.getList(
      '/api/devices/$normalizedServer/gateway/seen',
    );
    return response
        .map((item) => Map<String, dynamic>.from(item as Map))
        .toList();
  }
}
