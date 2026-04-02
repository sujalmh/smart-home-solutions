import 'package:equatable/equatable.dart';

class Client extends Equatable {
  static const int defaultModuleCount = 3;

  final String clientId;
  final String serverId;
  final String ip;
  final int moduleCount;
  final bool? online;

  const Client({
    required this.clientId,
    required this.serverId,
    required this.ip,
    required this.moduleCount,
    this.online,
  });

  factory Client.fromJson(Map<String, dynamic> json) {
    return Client(
      clientId: json['client_id'] as String,
      serverId: json['server_id'] as String,
      ip: json['ip'] as String,
      moduleCount:
          (json['module_count'] as num?)?.toInt() ?? defaultModuleCount,
      online: json['online'] as bool?,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'client_id': clientId,
      'server_id': serverId,
      'ip': ip,
      'module_count': moduleCount,
      'online': online,
    };
  }

  @override
  List<Object?> get props => [clientId, serverId, ip, moduleCount, online];
}
