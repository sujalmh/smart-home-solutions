import 'dart:async';
import 'dart:convert';

import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as ws_status;

class SocketClient {
  final String url;
  final String path;
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _reconnectTimer;
  bool _shouldReconnect = false;
  String? _token;

  final _responseController =
      StreamController<Map<String, dynamic>>.broadcast();
  final _statusController = StreamController<Map<String, dynamic>>.broadcast();

  SocketClient({required this.url, required this.path});

  Stream<Map<String, dynamic>> get responses => _responseController.stream;
  Stream<Map<String, dynamic>> get status => _statusController.stream;

  void connect({String? token}) {
    _token = token;
    _shouldReconnect = true;
    _connect();
  }

  void disconnect() {
    _shouldReconnect = false;
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
    _subscription?.cancel();
    _subscription = null;
    _channel?.sink.close(ws_status.goingAway);
    _channel = null;
  }

  void emit(String event, Map<String, dynamic> data) {
    final channel = _channel;
    if (channel == null) {
      return;
    }
    final message = jsonEncode({'event': event, 'data': data});
    channel.sink.add(message);
  }

  void dispose() {
    disconnect();
    _responseController.close();
    _statusController.close();
  }

  void _connect() {
    if (_channel != null) {
      return;
    }
    final uri = _buildUri(_token);
    _channel = WebSocketChannel.connect(uri);
    _subscription = _channel!.stream.listen(
      _handleMessage,
      onDone: _handleDisconnect,
      onError: (_) => _handleDisconnect(),
    );
  }

  void _handleDisconnect() {
    _subscription?.cancel();
    _subscription = null;
    _channel = null;
    if (_shouldReconnect) {
      _scheduleReconnect();
    }
  }

  void _scheduleReconnect() {
    if (_reconnectTimer?.isActive ?? false) {
      return;
    }
    _reconnectTimer = Timer(const Duration(seconds: 3), () {
      if (_shouldReconnect) {
        _connect();
      }
    });
  }

  Uri _buildUri(String? token) {
    final base = Uri.parse(url);
    final scheme = switch (base.scheme) {
      'https' => 'wss',
      'http' => 'ws',
      '' => 'wss',
      _ => base.scheme,
    };
    final resolvedPath = path.isEmpty
        ? (base.path.isEmpty ? '/ws' : base.path)
        : (path.startsWith('/') ? path : '/$path');
    final query = Map<String, String>.from(base.queryParameters);
    if (token != null && token.isNotEmpty) {
      query['token'] = token;
    }
    return base.replace(
      scheme: scheme,
      path: resolvedPath,
      queryParameters: query,
    );
  }

  void _handleMessage(dynamic message) {
    final text = _asString(message);
    if (text == null) {
      return;
    }
    final decoded = _safeDecode(text);
    if (decoded == null) {
      return;
    }
    if (decoded is! Map) {
      return;
    }
    final event = decoded['event'];
    final data = decoded['data'];
    if (event == 'response' && data is Map) {
      _responseController.add(_asMap(data));
    } else if (event == 'staresult' && data is Map) {
      _statusController.add(_asMap(data));
    }
  }

  String? _asString(dynamic message) {
    if (message is String) {
      return message;
    }
    if (message is List<int>) {
      return utf8.decode(message);
    }
    return null;
  }

  Map<String, dynamic>? _safeDecode(String payload) {
    try {
      final decoded = jsonDecode(payload);
      if (decoded is Map<String, dynamic>) {
        return decoded;
      }
      if (decoded is Map) {
        return decoded.map((key, value) => MapEntry(key.toString(), value));
      }
    } catch (_) {
      return null;
    }
    return null;
  }

  Map<String, dynamic> _asMap(dynamic data) {
    if (data is Map<String, dynamic>) {
      return data;
    }
    if (data is Map) {
      return data.map((key, value) => MapEntry(key.toString(), value));
    }
    return {};
  }
}
