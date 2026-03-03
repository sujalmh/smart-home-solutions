import '../../models/server.dart';
import '../../utils/id_utils.dart';
import '../api_client.dart';

class ServerRepository {
  final ApiClient api;

  ServerRepository({required this.api});

  Future<List<Server>> listServers() async {
    final response = await api.getList('/api/servers');
    return response
        .map((item) => Server.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<Server> createServer({
    required String serverId,
    required String password,
    required String ip,
  }) async {
    final response = await api.postJson('/api/servers', {
      'server_id': normalizeId(serverId),
      'pwd': password,
      'ip': ip,
    });
    return Server.fromJson(response);
  }
}
