class Server {
  final String serverId;
  final String ip;

  const Server({required this.serverId, required this.ip});

  factory Server.fromJson(Map<String, dynamic> json) {
    return Server(
      serverId: json['server_id'] as String,
      ip: json['ip'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {'server_id': serverId, 'ip': ip};
  }
}
