class Client {
  final String clientId;
  final String serverId;
  final String ip;
  final int moduleCount;

  const Client({
    required this.clientId,
    required this.serverId,
    required this.ip,
    required this.moduleCount,
  });

  factory Client.fromJson(Map<String, dynamic> json) {
    return Client(
      clientId: json['client_id'] as String,
      serverId: json['server_id'] as String,
      ip: json['ip'] as String,
      moduleCount: (json['module_count'] as num?)?.toInt() ?? 4,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'client_id': clientId,
      'server_id': serverId,
      'ip': ip,
      'module_count': moduleCount,
    };
  }
}
