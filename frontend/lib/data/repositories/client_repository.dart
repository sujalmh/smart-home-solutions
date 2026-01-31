import '../../models/client.dart';
import '../../models/switch_module.dart';
import '../api_client.dart';

class ClientRepository {
  final ApiClient api;

  ClientRepository({required this.api});

  Future<List<Client>> listClients({String? serverId}) async {
    final response = await api.getList(
      '/api/clients',
      query: serverId == null ? null : {'server_id': serverId},
    );
    return response
        .map((item) => Client.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<Client> createClient({
    required String clientId,
    required String serverId,
    required String password,
    required String ip,
  }) async {
    final response = await api.postJson('/api/clients', {
      'client_id': clientId,
      'server_id': serverId,
      'pwd': password,
      'ip': ip,
    });
    return Client.fromJson(response);
  }

  Future<List<SwitchModule>> listModules(String clientId) async {
    final response = await api.getList('/api/clients/$clientId/modules');
    return response
        .map((item) => SwitchModule.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<SwitchModule> updateModule({
    required String clientId,
    required String compId,
    required int mode,
    required int status,
    required int value,
  }) async {
    final response = await api.putJson(
      '/api/clients/$clientId/modules/$compId',
      {'mode': mode, 'status': status, 'value': value},
    );
    return SwitchModule.fromJson(response);
  }
}
