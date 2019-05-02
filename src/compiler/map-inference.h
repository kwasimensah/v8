// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_MAP_INFERENCE_H_
#define V8_COMPILER_MAP_INFERENCE_H_

#include "include/v8config.h"
#include "src/compiler/graph-reducer.h"
#include "src/objects/instance-type.h"
#include "src/objects/map.h"

namespace v8 {
namespace internal {

class VectorSlotPair;

namespace compiler {

class CompilationDependencies;
class JSGraph;
class JSHeapBroker;
class Node;

// The MapInference class provides access to the "inferred" maps of an
// {object}. This information can be either "reliable", meaning that the object
// is guaranteed to have one of these maps at runtime, or "unreliable", meaning
// that the object is guaranteed to have HAD one of these maps.
//
// The MapInference class does not expose whether or not the information is
// reliable. A client is expected to eventually make the information reliable by
// calling one of several methods that will either insert map checks, or record
// stability dependencies (or do nothing if the information was already
// reliable).
class MapInference {
 public:
  MapInference(JSHeapBroker* broker, Node* object, Node* effect);

  // The destructor checks that the information has been made reliable (if
  // necessary) and force-crashes if not.
  ~MapInference();

  // Is there any information at all?
  V8_WARN_UNUSED_RESULT bool HaveMaps() const;

  // These queries don't require a guard.
  //
  V8_WARN_UNUSED_RESULT bool AllOfInstanceTypesAreJSReceiver() const;
  // Here, {type} must not be a String type.
  V8_WARN_UNUSED_RESULT bool AllOfInstanceTypesAre(InstanceType type) const;

  // These queries require a guard. (Even instance types are generally not
  // reliable because of how the representation of a string can change.)
  V8_WARN_UNUSED_RESULT MapHandles const& GetMaps();
  V8_WARN_UNUSED_RESULT bool AllOfInstanceTypes(
      std::function<bool(InstanceType)> f);

  // These methods provide a guard.
  //
  // Returns true iff maps were already reliable or stability dependencies were
  // successfully recorded.
  V8_WARN_UNUSED_RESULT bool RelyOnMapsViaStability(
      CompilationDependencies* dependencies);
  // Records stability dependencies if possible, otherwise it inserts map
  // checks. Does nothing if maps were already reliable. Returns true iff
  // dependencies were taken.
  bool RelyOnMapsPreferStability(CompilationDependencies* dependencies,
                                 JSGraph* jsgraph, Node** effect, Node* control,
                                 const VectorSlotPair& feedback);
  // Inserts map checks even if maps were already reliable.
  void InsertMapChecks(JSGraph* jsgraph, Node** effect, Node* control,
                       const VectorSlotPair& feedback);

  // Internally marks the maps as reliable (thus bypassing the safety check) and
  // returns the NoChange reduction. USE THIS ONLY WHEN RETURNING, e.g.:
  //   if (foo) return inference.NoChange();
  V8_WARN_UNUSED_RESULT Reduction NoChange();

 private:
  JSHeapBroker* const broker_;
  Node* const object_;

  MapHandles maps_;
  enum {
    kReliableOrGuarded,
    kUnreliableDontNeedGuard,
    kUnreliableNeedGuard
  } maps_state_;

  bool Safe() const;
  void SetNeedGuardIfUnreliable();
  void SetGuarded();

  V8_WARN_UNUSED_RESULT bool AllOfInstanceTypesUnsafe(
      std::function<bool(InstanceType)> f) const;
  V8_WARN_UNUSED_RESULT bool RelyOnMapsHelper(
      CompilationDependencies* dependencies, JSGraph* jsgraph, Node** effect,
      Node* control, const VectorSlotPair& feedback);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_MAP_INFERENCE_H_
