import '../../models/device_command.dart';
import '../../models/switch_module.dart';
import '../api_client.dart';

class DeviceRepository {
  final ApiClient api;

  DeviceRepository({required this.api});

  Future<DeviceCommand> sendCommand(DeviceCommand command) async {
    final response = await api.postJson(
      '/api/devices/${command.serverId}/command',
      command.toJson(),
    );
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
    final response = await api.postList('/api/devices/$serverId/status', {
      'serverID': serverId,
      'devID': devId,
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
    await api.postJson('/api/devices/$serverId/config/server', {
      'server_id': serverId,
      if (password != null) 'pwd': password,
      if (ip != null) 'ip': ip,
    });
  }

  Future<void> configureRemote({
    required String serverId,
    required String ssid,
    required String password,
  }) async {
    await api.postJson('/api/devices/$serverId/config/remote', {
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
    await api.postJson('/api/devices/$serverId/config/client', {
      'client_id': clientId,
      'pwd': password,
      'ip': ip,
    });
  }

  Future<void> registerDevice({required String serverId}) async {
    await api.postJson('/api/devices/$serverId/register', {
      'server_id': serverId,
    });
  }
}
