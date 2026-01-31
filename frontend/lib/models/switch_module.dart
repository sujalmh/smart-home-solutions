class SwitchModule {
  final String clientId;
  final String compId;
  final int mode;
  final int status;
  final int value;

  const SwitchModule({
    required this.clientId,
    required this.compId,
    required this.mode,
    required this.status,
    required this.value,
  });

  factory SwitchModule.fromJson(Map<String, dynamic> json) {
    return SwitchModule(
      clientId: json['client_id'] as String,
      compId: json['comp_id'] as String,
      mode: (json['mode'] as num).toInt(),
      status: (json['status'] as num).toInt(),
      value: (json['value'] as num).toInt(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'client_id': clientId,
      'comp_id': compId,
      'mode': mode,
      'status': status,
      'value': value,
    };
  }
}
