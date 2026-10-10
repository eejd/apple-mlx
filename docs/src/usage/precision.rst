.. _precision:

Numerical Precision
===================

By default, MLX may run ``float32`` matrix-multiplication family
operations (matmul, quantized matmul, grouped matmul, convolution and
attention) at reduced precision on hardware with dedicated
matrix-multiplication units. Inputs and outputs stay ``float32``, but
results can differ from a full-precision reference by several orders of
magnitude more than ``float32`` rounding alone would explain.

To keep these operations in full ``float32``, set
:envvar:`MLX_ENABLE_TF32` to ``0`` when launching the process:

.. code-block:: shell

  MLX_ENABLE_TF32=0 python my_script.py

Which operations take the reduced-precision path, and how large the
difference is, depends on the backend and the hardware.

On Metal, ``conv2d`` can use the Winograd algorithm for 3x3 convolutions
with stride 1. Winograd is faster for large inputs, but its rounding error
in ``float32`` can be approximately 10 times larger. MLX selects Winograd
from the total input size, which includes the batch size. Thus, the same
image can give different results in a batch of 3 and in a batch of 1.

To disable Winograd, set :envvar:`MLX_CONV_WINOGRAD` to ``0``:

.. code-block:: shell

  MLX_CONV_WINOGRAD=0 python my_script.py

The setting can also be changed while the program runs, for the whole process
or for a block of code in the current thread:

.. code-block:: python

  mx.config.update("MLX_ENABLE_TF32", 0)  # full float32 from here on

  with mx.config.scoped_update(MLX_ENABLE_TF32=1):
      y = a @ b
      mx.eval(y)

The precision is chosen when an operation is evaluated, not when it is
recorded, so evaluate inside the block any result that should use the scoped
setting.
