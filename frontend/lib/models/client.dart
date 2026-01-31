class Client {
  final String clientId;
  final String serverId;
  final String ip;

  const Client({
    required this.clientId,
    required this.serverId,
    required this.ip,
  });

  factory Client.fromJson(Map<String, dynamic> json) {
    return Client(
      clientId: json['client_id'] as String,
      serverId: json['server_id'] as String,
      ip: json['ip'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {'client_id': clientId, 'server_id': serverId, 'ip': ip};
  }
}
