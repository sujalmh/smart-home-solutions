import 'dart:async';
import 'dart:convert';
import 'dart:math';

import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as ws_status;

enum SocketConnectionState { disconnected, connecting, reconnecting, connected }

class SocketClient {
  final String url;
  final String path;
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _reconnectTimer;
  bool _shouldReconnect = false;
  String? _token;

  // ── Exponential backoff state ────────────────────────────────────────
  static const int _baseDelayMs = 2000;
  static const int _maxDelayMs = 60000;
  static const double _jitterFactor = 0.25;
  int _reconnectAttempts = 0;
  final _random = Random();

  final _responseController =
      StreamController<Map<String, dynamic>>.broadcast();
  final _statusController = StreamController<Map<String, dynamic>>.broadcast();
  final _connectionController =
      StreamController<SocketConnectionState>.broadcast();
  final _gatewayStatusController =
      StreamController<Map<String, dynamic>>.broadcast();

  SocketClient({required this.url, required this.path});

  Stream<Map<String, dynamic>> get responses => _responseController.stream;
  Stream<Map<String, dynamic>> get status => _statusController.stream;
  Stream<SocketConnectionState> get connectionState =>
      _connectionController.stream;
  /// Emits a map with `{"serverID": String, "online": bool}` whenever a
  /// gateway connects or disconnects from the backend.
  Stream<Map<String, dynamic>> get gatewayStatus =>
      _gatewayStatusController.stream;

  void connect({String? token}) {
    _token = token;
    _shouldReconnect = true;
    _emitConnection(SocketConnectionState.connecting);
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
    _emitConnection(SocketConnectionState.disconnected);
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
    _connectionController.close();
    _gatewayStatusController.close();
  }

  void _connect() {
    if (_channel != null) {
      return;
    }
    final uri = _buildUri(_token);
    _channel = WebSocketChannel.connect(uri);
    unawaited(
      _channel!.ready
          .then((_) {
            _reconnectAttempts = 0; // reset backoff on success
            _emitConnection(SocketConnectionState.connected);
          })
          .catchError((_) {}),
    );
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
      _emitConnection(SocketConnectionState.reconnecting);
      _scheduleReconnect();
    } else {
      _emitConnection(SocketConnectionState.disconnected);
    }
  }

  void _scheduleReconnect() {
    if (_reconnectTimer?.isActive ?? false) {
      return;
    }
    // Exponential backoff: base × 2^attempt, capped at max, ±25% jitter.
    final exponential = _baseDelayMs * pow(2, _reconnectAttempts);
    final capped = min(exponential, _maxDelayMs).toInt();
    final jitter = (capped * _jitterFactor * (2 * _random.nextDouble() - 1))
        .round();
    final delay = Duration(milliseconds: capped + jitter);
    _reconnectAttempts++;
    _reconnectTimer = Timer(delay, () {
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
    final event = decoded['event'];
    final data = decoded['data'];
    if (event == 'response' && data is Map) {
      _responseController.add(_asMap(data));
    } else if (event == 'staresult' && data is Map) {
      _statusController.add(_asMap(data));
    } else if (event == 'gateway_status_changed' && data is Map) {
      _gatewayStatusController.add(_asMap(data));
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

  void _emitConnection(SocketConnectionState state) {
    if (_connectionController.isClosed) {
      return;
    }
    _connectionController.add(state);
  }
}
