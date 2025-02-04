#!/usr/bin/env python3
###################################################################################################
###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 # (now owned by Analog Devices, Inc.),
 # Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 # is proprietary to Analog Devices, Inc. and its licensors.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################
"""Functions for image processing
"""

import copy
import numpy as np
from PyQt5.QtGui import QImage


def cvt_img_to_qimage(img):
    """Converts OpenCv image to PyQt QImage
    """
    height, width, num_channles = img.shape
    bytes_per_line = num_channles * width
    return QImage(img.data, width, height, bytes_per_line, QImage.Format_RGB888)


def cvt_qimage_to_img(q_img, share_memory=False):
    """ Creates a numpy array from a QImage.

            If share_memory is True, the numpy array and the QImage is shared.
            Be careful: make sure the numpy array is destroyed before the image,
            otherwise the array will point to unreserved memory!!
    """
    assert isinstance(q_img, QImage), "img must be a QtGui.QImage object"
    assert q_img.format() == QImage.Format_RGB32, "img format must be QImage.Format.Format_RGB32, got: {}".format(q_img.format()) #pylint: disable=line-too-long

    img_size = q_img.size()
    buffer = q_img.constBits()
    buffer.setsize(img_size.height() * img_size.width() * 4)

    # Sanity check
    n_bits_buffer = len(buffer) * 8
    n_bits_image = img_size.width() * img_size.height() * q_img.depth()
    assert n_bits_buffer == n_bits_image, \
        "size mismatch: {} != {}".format(n_bits_buffer, n_bits_image)

    assert q_img.depth() == 32, "unexpected image depth: {}".format(q_img.depth())

    # Note the different width height parameter order!
    arr = np.ndarray(shape=(img_size.height(), img_size.width(), q_img.depth() // 8),
                     buffer=buffer,
                     dtype=np.uint8)
    arr = arr[:, :, :3]

    if share_memory:
        return arr

    return copy.deepcopy(arr)
