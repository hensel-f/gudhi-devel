# This file is part of the Gudhi Library - https://gudhi.inria.fr/ - which is released under MIT.
# See file LICENSE or go to https://gudhi.inria.fr/licensing/ for full license details.
# Author(s):    Felix Hensel, Mathieu Carrière
#
# Copyright (C) 2022 Inria
#
# Modification(s):
#   - YYYY/MM Author: Description of the modification

import tensorflow as tf

class DenseRagged(tf.keras.layers.Layer):
    """
    This is a class for the ragged layer in the RipsNet architecture, processing the input pointclouds.
    """

    def __init__(self, units, input_dim=None, use_bias=True, activation='gelu', **kwargs):
        """
        Constructor for the DenseRagged class.

        Parameters:
            units (int): number of units in the layer.
            use_bias (bool): flag, indicating whether to use bias or not.
            activation (string or function): identifier of a keras activation function, e.g. 'relu'.
        """
        super().__init__(dynamic=True, **kwargs)
        self._supports_ragged_inputs = True
        self.units = units
        self.use_bias = use_bias
        self.activation = tf.keras.activations.get(activation)

    def build(self, input_shape):
        last_dim = input_shape[-1]
        self.kernel = self.add_weight('kernel', shape=[last_dim, self.units], trainable=True)
        if self.use_bias:
            self.bias = self.add_weight('bias', shape=[self.units, ], trainable=True)
        else:
            self.bias = None
        super().build(input_shape)

    def call(self, inputs):
        """
        Apply DenseRagged layer on a ragged input tensor.

        Parameters:
            ragged tensor (e.g. containing a point cloud).

        Returns:
            ragged tensor containing the output of the layer.
        """
        outputs = tf.ragged.map_flat_values(tf.matmul, inputs, self.kernel)
        if self.use_bias:
            outputs = tf.ragged.map_flat_values(tf.nn.bias_add, outputs, self.bias)
        outputs = tf.ragged.map_flat_values(self.activation, outputs)
        return outputs


class DenseRaggedBlock(tf.keras.layers.Layer):
    """
    This is a block of DenseRagged layers.
    """

    def __init__(self, dense_ragged_layers, **kwargs):
        """
        Constructor for the DenseRagged class.

        Parameters:
        dense_ragged_layers (list): a list of DenseRagged layers :class:`~gudhi.tensorflow.DenseRagged`.
        input_dim (int): dimension of the pointcloud, if the input consists of pointclouds.
        """
        super().__init__(dynamic=True, **kwargs)
        self._supports_ragged_inputs = True
        self.dr_layers = dense_ragged_layers

    def build(self, input_shape):
        return self

    def call(self, inputs):
        """
        Apply the sequence of DenseRagged layers on a ragged input tensor.

        Parameters:
        ragged tensor (e.g. containing a point cloud).

        Returns:
        ragged tensor containing the output of the sequence of layers.
        """
        outputs = inputs
        for dr_layer in self.dr_layers:
            outputs = dr_layer(outputs)
        return outputs


class TFBlock(tf.keras.layers.Layer):
    """
    This class is a block of tensorflow layers.
    """

    def __init__(self, layers, **kwargs):
        """
        Constructor for the TFBlock class.

        Parameters:
        dense_layers (list): a list of dense tensorflow layers.
        """
        super().__init__(dynamic=True, **kwargs)
        self.layers = layers

    def build(self, input_shape):
        super().build(input_shape)

    def call(self, inputs):
        """
        Apply the sequence of layers on an input tensor.

        Parameters:
        inputs: any input tensor.

        Returns:
        output tensor containing the output of the sequence of layers.
        """
        outputs = inputs
        for layer in self.layers:
            outputs = layer(outputs)
        return outputs


class PermopRagged(tf.keras.layers.Layer):
    """
    This is a class for the permutation invariant layer in the RipsNet architecture.
    """

    def __init__(self, perm_op, **kwargs):
        """
        Constructor for the PermopRagged class.

        Parameters:
            perm_op: permutation invariant function, such as `tf.math.reduce_sum`, `tf.math.reduce_mean`, `tf.math.reduce_max`, `tf.math.reduce_min`, or a custom function.
        """
        super().__init__(dynamic=True, **kwargs)
        self._supports_ragged_inputs = True
        self.pop = perm_op

    def build(self, input_shape):
        super().build(input_shape)

    def call(self, inputs):
        """
        Apply PermopRagged on an input tensor.
        """
        out = self.pop(inputs, axis=1)
        return out


class RipsNet(tf.keras.Model):
    """
    This is a TensorFlow model for estimating vectorizations of persistence diagrams of point clouds.
    This class implements the RipsNet described in the following article <https://arxiv.org/abs/2202.01725>.
    """

    def __init__(self, phi_1, perm_op, phi_2, input_dim, **kwargs):
        """
        Constructor for the RipsNet class.

        Parameters:
            phi_1 (layers): any block of DenseRagged layers. Can be :class:`~gudhi.tensorflow.DenseRaggedBlock`, or a custom block built from :class:`~gudhi.tensorflow.DenseRagged` layers.
            perm_op (layer): layer for the permutation invariant function. Can be :class:`~gudhi.tensorflow.PermopRagged`.
            phi_2 (layers): Can be any (block of) TensorFlow layer(s),  e.g. :class:`~gudhi.tensorflow.TFBlock`.
            input_dim (int): dimension of the input point clouds.
        """
        super().__init__(dynamic=True, **kwargs)
        self.phi_1 = phi_1
        self.pop = perm_op
        self.phi_2 = phi_2
        self.input_dim = input_dim

    def build(self, input_shape):
        return self

    def call(self, pointclouds):
        """
        Apply RipsNet on a ragged tensor containing a list of pointclouds.

        Parameters:
            point clouds (n x None x input_dimension): ragged tensor containing n pointclouds in dimension `input_dimension`. The second dimension is ragged since point clouds can have different numbers of points.

        Returns:
            output (n x output_shape): tensor containing predicted vectorizations of the persistence diagrams of pointclouds.
        """
        inputs = tf.keras.layers.InputLayer(input_shape=(None, self.input_dim), dtype="float32", ragged=True)(
            pointclouds)
        output = self.phi_1(inputs)
        output = self.pop(output)
        output = self.phi_2(output)
        return output