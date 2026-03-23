#ifndef __REDUCER_BYTECODE_IMPL_H__
#define __REDUCER_BYTECODE_IMPL_H__

#define ON_NODE_ARITY0                                                         \
  case CELLS_NODE_TYPE_VALUEF0:                                                \
  case CELLS_NODE_TYPE_VALUEV0:                                                \
  case CELLS_NODE_TYPE_DELTA0:

#define ON_NODE_ARITY1                                                         \
  case CELLS_NODE_TYPE_VALUEF1:                                                \
  case CELLS_NODE_TYPE_VALUEV1:                                                \
  case CELLS_NODE_TYPE_DELTA1:

#define ON_NODE_ARITY2                                                         \
  case CELLS_NODE_TYPE_VALUEF2:                                                \
  case CELLS_NODE_TYPE_VALUEV2:                                                \
  case CELLS_NODE_TYPE_DELTA2:

#endif // __REDUCER_BYTECODE_IMPL_H__
