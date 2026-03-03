import 'package:equatable/equatable.dart';

class AIRoomRef extends Equatable {
  final String id;
  final String name;

  const AIRoomRef({required this.id, required this.name});

  factory AIRoomRef.fromJson(Map<String, dynamic> json) {
    return AIRoomRef(id: json['id'] as String, name: json['name'] as String);
  }

  @override
  List<Object?> get props => [id, name];
}

class AIDeviceRef extends Equatable {
  final String id;
  final String name;
  final String type;

  const AIDeviceRef({required this.id, required this.name, required this.type});

  factory AIDeviceRef.fromJson(Map<String, dynamic> json) {
    return AIDeviceRef(
      id: json['id'] as String,
      name: json['name'] as String,
      type: json['type'] as String,
    );
  }

  @override
  List<Object?> get props => [id, name, type];
}

class AIIntent extends Equatable {
  final String action;
  final double confidence;

  const AIIntent({required this.action, required this.confidence});

  factory AIIntent.fromJson(Map<String, dynamic> json) {
    return AIIntent(
      action: json['action'] as String,
      confidence: (json['confidence'] as num).toDouble(),
    );
  }

  @override
  List<Object?> get props => [action, confidence];
}

class AIResolvedEntities extends Equatable {
  final AIRoomRef? room;
  final List<AIDeviceRef> devices;
  final String action;
  final int? value;
  final String? comp;

  const AIResolvedEntities({
    required this.room,
    required this.devices,
    required this.action,
    required this.value,
    required this.comp,
  });

  factory AIResolvedEntities.fromJson(Map<String, dynamic> json) {
    final roomJson = json['room'] as Map<String, dynamic>?;
    final devicesJson = (json['devices'] as List<dynamic>? ?? <dynamic>[])
        .cast<Map<String, dynamic>>();
    return AIResolvedEntities(
      room: roomJson == null ? null : AIRoomRef.fromJson(roomJson),
      devices: devicesJson.map(AIDeviceRef.fromJson).toList(),
      action: json['action'] as String,
      value: (json['value'] as num?)?.toInt(),
      comp: json['comp'] as String?,
    );
  }

  @override
  List<Object?> get props => [room, devices, action, value, comp];
}

class AIExecutionPlan extends Equatable {
  final String action;
  final AIRoomRef? room;
  final List<AIDeviceRef> devices;
  final int? value;
  final String comp;
  final String source;

  const AIExecutionPlan({
    required this.action,
    required this.room,
    required this.devices,
    required this.value,
    required this.comp,
    required this.source,
  });

  factory AIExecutionPlan.fromJson(Map<String, dynamic> json) {
    final roomJson = json['room'] as Map<String, dynamic>?;
    final devicesJson = (json['devices'] as List<dynamic>? ?? <dynamic>[])
        .cast<Map<String, dynamic>>();
    return AIExecutionPlan(
      action: json['action'] as String,
      room: roomJson == null ? null : AIRoomRef.fromJson(roomJson),
      devices: devicesJson.map(AIDeviceRef.fromJson).toList(),
      value: (json['value'] as num?)?.toInt(),
      comp: json['comp'] as String,
      source: json['source'] as String,
    );
  }

  @override
  List<Object?> get props => [action, room, devices, value, comp, source];
}

class AIToolResult extends Equatable {
  final String status;
  final bool executed;
  final List<String> reasonCodes;
  final List<AIToolExecutionItem> items;

  const AIToolResult({
    required this.status,
    required this.executed,
    required this.reasonCodes,
    required this.items,
  });

  factory AIToolResult.fromJson(Map<String, dynamic> json) {
    final reasons = (json['reason_codes'] as List<dynamic>? ?? <dynamic>[])
        .map((item) => item as String)
        .toList();
    final parsedItems = (json['items'] as List<dynamic>? ?? <dynamic>[])
        .cast<Map<String, dynamic>>()
        .map(AIToolExecutionItem.fromJson)
        .toList();
    return AIToolResult(
      status: json['status'] as String,
      executed: json['executed'] as bool,
      reasonCodes: reasons,
      items: parsedItems,
    );
  }

  @override
  List<Object?> get props => [status, executed, reasonCodes, items];
}

class AIToolExecutionItem extends Equatable {
  final AIDeviceRef device;
  final String status;
  final String detail;

  const AIToolExecutionItem({
    required this.device,
    required this.status,
    required this.detail,
  });

  factory AIToolExecutionItem.fromJson(Map<String, dynamic> json) {
    return AIToolExecutionItem(
      device: AIDeviceRef.fromJson(json['device'] as Map<String, dynamic>),
      status: json['status'] as String,
      detail: json['detail'] as String,
    );
  }

  @override
  List<Object?> get props => [device, status, detail];
}

class AIFinalResponse extends Equatable {
  final String status;
  final String reply;
  final bool requiresClarification;
  final bool requiresConfirmation;
  final AIIntent intent;
  final AIResolvedEntities entities;
  final AIExecutionPlan? plan;
  final AIToolResult? result;

  const AIFinalResponse({
    required this.status,
    required this.reply,
    required this.requiresClarification,
    required this.requiresConfirmation,
    required this.intent,
    required this.entities,
    required this.plan,
    required this.result,
  });

  factory AIFinalResponse.fromJson(Map<String, dynamic> json) {
    final planJson = json['plan'] as Map<String, dynamic>?;
    final resultJson = json['result'] as Map<String, dynamic>?;
    return AIFinalResponse(
      status: json['status'] as String,
      reply: json['reply'] as String,
      requiresClarification: json['requires_clarification'] as bool? ?? false,
      requiresConfirmation: json['requires_confirmation'] as bool? ?? false,
      intent: AIIntent.fromJson(json['intent'] as Map<String, dynamic>),
      entities: AIResolvedEntities.fromJson(
        json['entities'] as Map<String, dynamic>,
      ),
      plan: planJson == null ? null : AIExecutionPlan.fromJson(planJson),
      result: resultJson == null ? null : AIToolResult.fromJson(resultJson),
    );
  }

  @override
  List<Object?> get props => [
    status,
    reply,
    requiresClarification,
    requiresConfirmation,
    intent,
    entities,
    plan,
    result,
  ];
}

class AIChatResponse extends Equatable {
  final String conversationId;
  final AIFinalResponse response;
  final DateTime? expiresAt;

  const AIChatResponse({
    required this.conversationId,
    required this.response,
    required this.expiresAt,
  });

  factory AIChatResponse.fromJson(Map<String, dynamic> json) {
    final expires = json['expires_at'] as String?;
    return AIChatResponse(
      conversationId: json['conversation_id'] as String,
      response: AIFinalResponse.fromJson(
        json['response'] as Map<String, dynamic>,
      ),
      expiresAt: expires == null ? null : DateTime.parse(expires),
    );
  }

  @override
  List<Object?> get props => [conversationId, response, expiresAt];
}
