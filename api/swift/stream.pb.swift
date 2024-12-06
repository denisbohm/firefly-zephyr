// DO NOT EDIT.
// swift-format-ignore-file
//
// Generated by the Swift generator plugin for the protocol buffer compiler.
// Source: stream.proto
//
// For information on using the generated types, please see the documentation:
//   https://github.com/apple/swift-protobuf/

import Foundation
import SwiftProtobuf

// If the compiler emits an error on this type, it is because this file
// was generated by a version of the `protoc` Swift plug-in that is
// incompatible with the version of SwiftProtobuf to which you are linking.
// Please ensure that you are building against the same version of the API
// that was used to generate this file.
fileprivate struct _GeneratedWithProtocGenSwiftVersion: SwiftProtobuf.ProtobufAPIVersionCheck {
  struct _2: SwiftProtobuf.ProtobufAPIVersion_2 {}
  typealias Version = _2
}

struct Firefly_Stream_V1_Value {
  // SwiftProtobuf.Message conformance is added in an extension below. See the
  // `Message` and `Message+*Additions` files in the SwiftProtobuf library for
  // methods supported on all messages.

  var kind: Firefly_Stream_V1_Value.OneOf_Kind? = nil

  var boolValue: Bool {
    get {
      if case .boolValue(let v)? = kind {return v}
      return false
    }
    set {kind = .boolValue(newValue)}
  }

  var uint32Value: UInt32 {
    get {
      if case .uint32Value(let v)? = kind {return v}
      return 0
    }
    set {kind = .uint32Value(newValue)}
  }

  var int32Value: Int32 {
    get {
      if case .int32Value(let v)? = kind {return v}
      return 0
    }
    set {kind = .int32Value(newValue)}
  }

  var floatValue: Float {
    get {
      if case .floatValue(let v)? = kind {return v}
      return 0
    }
    set {kind = .floatValue(newValue)}
  }

  var unknownFields = SwiftProtobuf.UnknownStorage()

  enum OneOf_Kind: Equatable {
    case boolValue(Bool)
    case uint32Value(UInt32)
    case int32Value(Int32)
    case floatValue(Float)

  #if !swift(>=4.1)
    static func ==(lhs: Firefly_Stream_V1_Value.OneOf_Kind, rhs: Firefly_Stream_V1_Value.OneOf_Kind) -> Bool {
      // The use of inline closures is to circumvent an issue where the compiler
      // allocates stack space for every case branch when no optimizations are
      // enabled. https://github.com/apple/swift-protobuf/issues/1034
      switch (lhs, rhs) {
      case (.boolValue, .boolValue): return {
        guard case .boolValue(let l) = lhs, case .boolValue(let r) = rhs else { preconditionFailure() }
        return l == r
      }()
      case (.uint32Value, .uint32Value): return {
        guard case .uint32Value(let l) = lhs, case .uint32Value(let r) = rhs else { preconditionFailure() }
        return l == r
      }()
      case (.int32Value, .int32Value): return {
        guard case .int32Value(let l) = lhs, case .int32Value(let r) = rhs else { preconditionFailure() }
        return l == r
      }()
      case (.floatValue, .floatValue): return {
        guard case .floatValue(let l) = lhs, case .floatValue(let r) = rhs else { preconditionFailure() }
        return l == r
      }()
      default: return false
      }
    }
  #endif
  }

  init() {}
}

struct Firefly_Stream_V1_ConnectionConfiguration {
  // SwiftProtobuf.Message conformance is added in an extension below. See the
  // `Message` and `Message+*Additions` files in the SwiftProtobuf library for
  // methods supported on all messages.

  var maximumFlowWindow: Firefly_Stream_V1_Value {
    get {return _maximumFlowWindow ?? Firefly_Stream_V1_Value()}
    set {_maximumFlowWindow = newValue}
  }
  /// Returns true if `maximumFlowWindow` has been explicitly set.
  var hasMaximumFlowWindow: Bool {return self._maximumFlowWindow != nil}
  /// Clears the value of `maximumFlowWindow`. Subsequent reads from it will return its default value.
  mutating func clearMaximumFlowWindow() {self._maximumFlowWindow = nil}

  var maximumTransmissionUnit: Firefly_Stream_V1_Value {
    get {return _maximumTransmissionUnit ?? Firefly_Stream_V1_Value()}
    set {_maximumTransmissionUnit = newValue}
  }
  /// Returns true if `maximumTransmissionUnit` has been explicitly set.
  var hasMaximumTransmissionUnit: Bool {return self._maximumTransmissionUnit != nil}
  /// Clears the value of `maximumTransmissionUnit`. Subsequent reads from it will return its default value.
  mutating func clearMaximumTransmissionUnit() {self._maximumTransmissionUnit = nil}

  var receiveKeepAlive: Firefly_Stream_V1_Value {
    get {return _receiveKeepAlive ?? Firefly_Stream_V1_Value()}
    set {_receiveKeepAlive = newValue}
  }
  /// Returns true if `receiveKeepAlive` has been explicitly set.
  var hasReceiveKeepAlive: Bool {return self._receiveKeepAlive != nil}
  /// Clears the value of `receiveKeepAlive`. Subsequent reads from it will return its default value.
  mutating func clearReceiveKeepAlive() {self._receiveKeepAlive = nil}

  var sendKeepAlive: Firefly_Stream_V1_Value {
    get {return _sendKeepAlive ?? Firefly_Stream_V1_Value()}
    set {_sendKeepAlive = newValue}
  }
  /// Returns true if `sendKeepAlive` has been explicitly set.
  var hasSendKeepAlive: Bool {return self._sendKeepAlive != nil}
  /// Clears the value of `sendKeepAlive`. Subsequent reads from it will return its default value.
  mutating func clearSendKeepAlive() {self._sendKeepAlive = nil}

  var sendRetryTimeout: Firefly_Stream_V1_Value {
    get {return _sendRetryTimeout ?? Firefly_Stream_V1_Value()}
    set {_sendRetryTimeout = newValue}
  }
  /// Returns true if `sendRetryTimeout` has been explicitly set.
  var hasSendRetryTimeout: Bool {return self._sendRetryTimeout != nil}
  /// Clears the value of `sendRetryTimeout`. Subsequent reads from it will return its default value.
  mutating func clearSendRetryTimeout() {self._sendRetryTimeout = nil}

  var unknownFields = SwiftProtobuf.UnknownStorage()

  init() {}

  fileprivate var _maximumFlowWindow: Firefly_Stream_V1_Value? = nil
  fileprivate var _maximumTransmissionUnit: Firefly_Stream_V1_Value? = nil
  fileprivate var _receiveKeepAlive: Firefly_Stream_V1_Value? = nil
  fileprivate var _sendKeepAlive: Firefly_Stream_V1_Value? = nil
  fileprivate var _sendRetryTimeout: Firefly_Stream_V1_Value? = nil
}

#if swift(>=5.5) && canImport(_Concurrency)
extension Firefly_Stream_V1_Value: @unchecked Sendable {}
extension Firefly_Stream_V1_Value.OneOf_Kind: @unchecked Sendable {}
extension Firefly_Stream_V1_ConnectionConfiguration: @unchecked Sendable {}
#endif  // swift(>=5.5) && canImport(_Concurrency)

// MARK: - Code below here is support for the SwiftProtobuf runtime.

fileprivate let _protobuf_package = "firefly.stream.v1"

extension Firefly_Stream_V1_Value: SwiftProtobuf.Message, SwiftProtobuf._MessageImplementationBase, SwiftProtobuf._ProtoNameProviding {
  static let protoMessageName: String = _protobuf_package + ".Value"
  static let _protobuf_nameMap: SwiftProtobuf._NameMap = [
    1: .standard(proto: "bool_value"),
    2: .standard(proto: "uint32_value"),
    3: .standard(proto: "int32_value"),
    4: .standard(proto: "float_value"),
  ]

  mutating func decodeMessage<D: SwiftProtobuf.Decoder>(decoder: inout D) throws {
    while let fieldNumber = try decoder.nextFieldNumber() {
      // The use of inline closures is to circumvent an issue where the compiler
      // allocates stack space for every case branch when no optimizations are
      // enabled. https://github.com/apple/swift-protobuf/issues/1034
      switch fieldNumber {
      case 1: try {
        var v: Bool?
        try decoder.decodeSingularBoolField(value: &v)
        if let v = v {
          if self.kind != nil {try decoder.handleConflictingOneOf()}
          self.kind = .boolValue(v)
        }
      }()
      case 2: try {
        var v: UInt32?
        try decoder.decodeSingularUInt32Field(value: &v)
        if let v = v {
          if self.kind != nil {try decoder.handleConflictingOneOf()}
          self.kind = .uint32Value(v)
        }
      }()
      case 3: try {
        var v: Int32?
        try decoder.decodeSingularInt32Field(value: &v)
        if let v = v {
          if self.kind != nil {try decoder.handleConflictingOneOf()}
          self.kind = .int32Value(v)
        }
      }()
      case 4: try {
        var v: Float?
        try decoder.decodeSingularFloatField(value: &v)
        if let v = v {
          if self.kind != nil {try decoder.handleConflictingOneOf()}
          self.kind = .floatValue(v)
        }
      }()
      default: break
      }
    }
  }

  func traverse<V: SwiftProtobuf.Visitor>(visitor: inout V) throws {
    // The use of inline closures is to circumvent an issue where the compiler
    // allocates stack space for every if/case branch local when no optimizations
    // are enabled. https://github.com/apple/swift-protobuf/issues/1034 and
    // https://github.com/apple/swift-protobuf/issues/1182
    switch self.kind {
    case .boolValue?: try {
      guard case .boolValue(let v)? = self.kind else { preconditionFailure() }
      try visitor.visitSingularBoolField(value: v, fieldNumber: 1)
    }()
    case .uint32Value?: try {
      guard case .uint32Value(let v)? = self.kind else { preconditionFailure() }
      try visitor.visitSingularUInt32Field(value: v, fieldNumber: 2)
    }()
    case .int32Value?: try {
      guard case .int32Value(let v)? = self.kind else { preconditionFailure() }
      try visitor.visitSingularInt32Field(value: v, fieldNumber: 3)
    }()
    case .floatValue?: try {
      guard case .floatValue(let v)? = self.kind else { preconditionFailure() }
      try visitor.visitSingularFloatField(value: v, fieldNumber: 4)
    }()
    case nil: break
    }
    try unknownFields.traverse(visitor: &visitor)
  }

  static func ==(lhs: Firefly_Stream_V1_Value, rhs: Firefly_Stream_V1_Value) -> Bool {
    if lhs.kind != rhs.kind {return false}
    if lhs.unknownFields != rhs.unknownFields {return false}
    return true
  }
}

extension Firefly_Stream_V1_ConnectionConfiguration: SwiftProtobuf.Message, SwiftProtobuf._MessageImplementationBase, SwiftProtobuf._ProtoNameProviding {
  static let protoMessageName: String = _protobuf_package + ".ConnectionConfiguration"
  static let _protobuf_nameMap: SwiftProtobuf._NameMap = [
    1: .standard(proto: "maximum_flow_window"),
    2: .standard(proto: "maximum_transmission_unit"),
    3: .standard(proto: "receive_keep_alive"),
    4: .standard(proto: "send_keep_alive"),
    5: .standard(proto: "send_retry_timeout"),
  ]

  mutating func decodeMessage<D: SwiftProtobuf.Decoder>(decoder: inout D) throws {
    while let fieldNumber = try decoder.nextFieldNumber() {
      // The use of inline closures is to circumvent an issue where the compiler
      // allocates stack space for every case branch when no optimizations are
      // enabled. https://github.com/apple/swift-protobuf/issues/1034
      switch fieldNumber {
      case 1: try { try decoder.decodeSingularMessageField(value: &self._maximumFlowWindow) }()
      case 2: try { try decoder.decodeSingularMessageField(value: &self._maximumTransmissionUnit) }()
      case 3: try { try decoder.decodeSingularMessageField(value: &self._receiveKeepAlive) }()
      case 4: try { try decoder.decodeSingularMessageField(value: &self._sendKeepAlive) }()
      case 5: try { try decoder.decodeSingularMessageField(value: &self._sendRetryTimeout) }()
      default: break
      }
    }
  }

  func traverse<V: SwiftProtobuf.Visitor>(visitor: inout V) throws {
    // The use of inline closures is to circumvent an issue where the compiler
    // allocates stack space for every if/case branch local when no optimizations
    // are enabled. https://github.com/apple/swift-protobuf/issues/1034 and
    // https://github.com/apple/swift-protobuf/issues/1182
    try { if let v = self._maximumFlowWindow {
      try visitor.visitSingularMessageField(value: v, fieldNumber: 1)
    } }()
    try { if let v = self._maximumTransmissionUnit {
      try visitor.visitSingularMessageField(value: v, fieldNumber: 2)
    } }()
    try { if let v = self._receiveKeepAlive {
      try visitor.visitSingularMessageField(value: v, fieldNumber: 3)
    } }()
    try { if let v = self._sendKeepAlive {
      try visitor.visitSingularMessageField(value: v, fieldNumber: 4)
    } }()
    try { if let v = self._sendRetryTimeout {
      try visitor.visitSingularMessageField(value: v, fieldNumber: 5)
    } }()
    try unknownFields.traverse(visitor: &visitor)
  }

  static func ==(lhs: Firefly_Stream_V1_ConnectionConfiguration, rhs: Firefly_Stream_V1_ConnectionConfiguration) -> Bool {
    if lhs._maximumFlowWindow != rhs._maximumFlowWindow {return false}
    if lhs._maximumTransmissionUnit != rhs._maximumTransmissionUnit {return false}
    if lhs._receiveKeepAlive != rhs._receiveKeepAlive {return false}
    if lhs._sendKeepAlive != rhs._sendKeepAlive {return false}
    if lhs._sendRetryTimeout != rhs._sendRetryTimeout {return false}
    if lhs.unknownFields != rhs.unknownFields {return false}
    return true
  }
}