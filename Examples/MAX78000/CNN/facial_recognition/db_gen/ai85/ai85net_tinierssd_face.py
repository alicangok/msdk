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
"""
Tiny SSD (Single Shot Detector) Variant Model for Face Detection
"""
from math import sqrt

import torch
import torch.nn.functional as F
from torch import nn

import ai85.ai8x as ai8x
import object_detection_utils as obj_detect_utils
#import torch as th
#th.autograd.set_detect_anomaly(True)

class TinySSDBaseFace(nn.Module):
    """
    Base convolutions to produce lower-level feature maps.
    """

    def __init__(self, **kwargs):
        super().__init__()

        # Standard convolutional layers
        self.fire1 = ai8x.FusedMaxPoolConv2dReLU(3, 16, 3, padding=1, bias=False)
        self.fire2 = ai8x.FusedMaxPoolConv2dReLU(16, 32, 3, padding=1, bias=False)

        self.fire3 = ai8x.FusedConv2dBNReLU(32, 64, 3, padding=1, **kwargs)
        self.fire4 = ai8x.FusedConv2dBNReLU(64, 64, 3, padding=1, **kwargs)

        self.fire5 = ai8x.FusedConv2dBNReLU(64, 64, 3, padding=1, **kwargs)

        self.fire6 = ai8x.FusedConv2dBNReLU(64, 64, 3, padding=1, **kwargs)
        self.fire7 = ai8x.FusedConv2dBNReLU(64, 128, 3, padding=1, **kwargs)
        self.fire8 = ai8x.FusedConv2dBNReLU(128, 32, 3, padding=1, **kwargs)

        self.fire9 = ai8x.FusedMaxPoolConv2dBNReLU(32, 32, 3, padding=1,
                                                   **kwargs)

        self.fire10 = ai8x.FusedMaxPoolConv2dBNReLU(32, 32, 3, padding=1, **kwargs)

    def forward(self, image):
        """
        Forward propagation.

        :param image: images, a tensor of dimensions
        :return: lower-level feature maps
        """
        out = self.fire1(image)  # (N, 32, 112, 84)
        out = self.fire2(out)  # (N, 32, 56, 42)

        out = self.fire3(out)  # (N, 64, 56, 42)

        fire4_feats = self.fire4(out)  # (N, 64, 56, 42)


        out = self.fire5(fire4_feats)  # (N, 64, 56, 42)

        out = self.fire6(out)  # (N, 64, 56, 42)

        out = self.fire7(out)  # (N, 128, 56, 42)

        fire8_feats = self.fire8(out)  # (N, 32, 56, 42)


        fire9_feats = self.fire9(fire8_feats)  # (N, 32, 28, 21)


        fire10_feats = self.fire10(fire9_feats)  # (N, 32, 14, 10)


        return fire4_feats, fire8_feats, fire9_feats, fire10_feats


class AuxiliaryConvolutions(nn.Module):
    """
    Additional convolutions to produce higher-level feature maps.
    """

    def __init__(self, **kwargs):
        super().__init__()

        # Auxiliary/additional convolutions on top of the VGG base
        self.conv12_1 = ai8x.FusedConv2dBNReLU(32, 16, 3, padding=1, **kwargs)  # (N, 16, 14, 10)
        self.conv12_2 = ai8x.FusedMaxPoolConv2dBNReLU(16, 16, 3, padding=1,  **kwargs)  # (N, 16, 7, 5)

        self.init_conv2d()

    def init_conv2d(self):
        """
        Initialize convolution parameters.
        """
        for c in self.children():
            if isinstance(c, nn.Conv2d):
                nn.init.xavier_uniform_(c.weight)
                nn.init.constant_(c.bias, 0.)

    def forward(self, fire10_feats):
        """
        Forward propagation.

        :param conv7_feats: lower-level feature map
        :return: higher-level feature maps
        """

        out = self.conv12_1(fire10_feats)

        conv12_2_feats = self.conv12_2(out)


        return conv12_2_feats


class PredictionConvolutions(nn.Module):
    """
    Convolutions to predict class scores and bounding boxes using lower and higher-level feature
    maps.

    The bounding boxes (locations) are predicted as encoded offsets w.r.t each of the prior
    (default) boxes.
    See 'cxcy_to_gcxgcy' in utils.py for the encoding definition.

    The class scores represent the scores of each object class in each of the bounding boxes
    located.
    A high score for 'background' = no object.
    """

    def __init__(self, n_classes, **kwargs):
        """
        :param n_classes: number of different types of objects
        """
        super().__init__()

        self.n_classes = n_classes

        n_boxes = {'fire9': 2,
                'conv12_2': 2}

        # 4 prior-boxes implies we use 4 different aspect ratios, etc.
        
        self.loc_fire9 = ai8x.FusedConv2dBN(32, n_boxes['fire9'] * 4, kernel_size=3, padding=1, 
                                            **kwargs)
        
        self.loc_conv12_2 = ai8x.FusedConv2dBN(16, n_boxes['conv12_2'] * 4, kernel_size=3, 
                                               padding=1, **kwargs)

        # Class prediction convolutions (predict classes in localization boxes)        
        self.cl_fire9 = ai8x.FusedConv2dBN(32, n_boxes['fire9'] * n_classes, kernel_size=3, 
                                           padding=1, **kwargs)
        
        self.cl_conv12_2 = ai8x.FusedConv2dBN(16, n_boxes['conv12_2'] * n_classes, kernel_size=3,
                                              padding=1, **kwargs)


        # Initialize convolutions' parameters
        self.init_conv2d()

    def init_conv2d(self):
        """
        Initialize convolution parameters.
        """
        for c in self.children():
            if isinstance(c, nn.Conv2d):
                nn.init.xavier_uniform_(c.weight)
                nn.init.constant_(c.bias, 0.)

    def forward(self, fire4_feats, fire9_feats,  conv12_2_feats):
        """
        Forward propagation.
        """
        batch_size = fire4_feats.size(0)

        

        l_fire9 = self.loc_fire9(fire9_feats)
        l_fire9 = l_fire9.permute(0, 2, 3, 1).contiguous()
        l_fire9 = l_fire9.view(batch_size, -1, 4)

        

        l_conv12_2 = self.loc_conv12_2(conv12_2_feats)
        l_conv12_2 = l_conv12_2.permute(0, 2, 3, 1).contiguous()
        l_conv12_2 = l_conv12_2.view(batch_size, -1, 4)


        c_fire9 = self.cl_fire9(fire9_feats)
        c_fire9 = c_fire9.permute(0, 2, 3, 1).contiguous()
        c_fire9 = c_fire9.view(batch_size, -1, self.n_classes)
        

        c_conv12_2 = self.cl_conv12_2(conv12_2_feats)
        c_conv12_2 = c_conv12_2.permute(0, 2, 3, 1).contiguous()
        c_conv12_2 = c_conv12_2.view(batch_size, -1, self.n_classes)

        # Concatenate in this specific order (i.e. must match the order of the prior-boxes)
        locs = torch.cat([l_fire9,  l_conv12_2], dim=1)
        classes_scores = torch.cat([ c_fire9,  c_conv12_2],
                                   dim=1)
        return (locs, classes_scores)


class TinierSSDFace(nn.Module):
    """
    The TinySSD network - encapsulates the base network, auxiliary, and prediction
    convolutions.
    """
    # Aspect ratios for the 2 prior boxes in each of the two feature map
    default_aspect_ratios = (
        (0.9, 0.75),
        (0.9, 0.75),
    )

    def __init__(self, num_classes,
                 num_channels=3,  # pylint: disable=unused-argument
                 dimensions=(168, 224),  # pylint: disable=unused-argument
                 aspect_ratios=default_aspect_ratios,
                 device='cpu',
                 **kwargs):
        super().__init__()

        self.n_classes = num_classes

        self.base = TinySSDBaseFace(**kwargs)
        self.aux_convs = AuxiliaryConvolutions(**kwargs)
        self.pred_convs = PredictionConvolutions(self.n_classes, **kwargs)

        # Prior boxes
        self.device = device
        self.priors_cxcy = self.__class__.create_prior_boxes(aspect_ratios=aspect_ratios,
                                                             device=self.device)

    def forward(self, image):
        """
        Forward propagation.

        :param image: images, a tensor of dimensions
        :return: Prior boxes' locations and class scores for each image
        """
        fire4_feats, fire8_feats, fire9_feats, fire10_feats = self.base(image)

        # Run auxiliary convolutions (higher level feature map generators)
        conv12_2_feats = self.aux_convs(fire10_feats)

        # Run prediction convolutions (predict offsets w.r.t prior-boxes and classes in each
        # resulting localization box)
        locs, classes_scores = self.pred_convs(fire4_feats, fire9_feats, conv12_2_feats)

        return locs, classes_scores

    @staticmethod
    def create_prior_boxes(aspect_ratios=default_aspect_ratios, device='cpu'):
        """
        Create the prior (default) boxes
        :return: prior boxes in center-size coordinates
        """

        fmap_dims = {'fire9': (28,21),
                     'conv12_2': (7,5)}

        fmaps = list(fmap_dims.keys())

        obj_scales = {'fire9': 0.35715,
                      'conv12_2': 0.7143}

        if len(aspect_ratios) != len(fmaps):
            raise ValueError(f'aspect_ratios list should have length {len(fmaps)}')

        if True in (len(aspect_ratios_list) !=
                    len(TinierSSDFace.default_aspect_ratios[0])
                    for aspect_ratios_list in aspect_ratios):
            raise ValueError(f'Each aspect_ratios list should have length \
                               {len(TinierSSDFace.default_aspect_ratios[0])}')

        aspect_ratios = {'fire9': aspect_ratios[0],
                         'conv12_2': aspect_ratios[1]}

        prior_boxes = []

        for k, fmap in enumerate(fmaps):
            for i in range(fmap_dims[fmap][0]):
                for j in range(fmap_dims[fmap][1]):
                    cx = (j + 0.5) / fmap_dims[fmap][1]
                    cy = (i + 0.5) / fmap_dims[fmap][0]

                    for ratio in aspect_ratios[fmap]:
                        prior_boxes.append([cx, cy, obj_scales[fmap] * sqrt(ratio),
                                            obj_scales[fmap] / sqrt(ratio)])

                        # For an aspect ratio of 1, use an additional prior whose scale is the
                        # geometric mean of the
                        # scale of the current feature map and the scale of the next feature map
                        if ratio == 1.:
                            try:
                                additional_scale = sqrt(obj_scales[fmap] *
                                                        obj_scales[fmaps[k + 1]])
                            # For the last feature map, there is no "next" feature map
                            except IndexError:
                                additional_scale = 1.
                            prior_boxes.append([cx, cy, additional_scale, additional_scale])
        prior_boxes = torch.FloatTensor(prior_boxes).to(device)  # INSERT SIZE HERE
        prior_boxes.clamp_(0, 1)  # INSERT SIZE HERE

        return prior_boxes

    def detect_objects(self, predicted_locs, predicted_scores, min_score, max_overlap, top_k):
        """
        Decipher the locations and class scores to detect objects.

        For each class, perform Non-Maximum Suppression (NMS) on boxes that are above a minimum
        threshold.

        :param predicted_locs: predicted locations/boxes w.r.t the prior boxes, a tensor of
        dimensions
        :param predicted_scores: class scores for each of the encoded locations/boxes, a tensor of
        dimensions
        :param min_score: minimum threshold for a box to be considered a match for a certain class
        :param max_overlap: maximum overlap two boxes can have so that the one with the lower score
        is not suppressed via NMS
        :param top_k: if there are a lot of resulting detection across all classes, keep only the
        top 'k'
        :return: detections (boxes, labels, and scores), lists of length batch_size
        """
        batch_size = predicted_locs.size(0)
        n_priors = self.priors_cxcy.size(0)
        predicted_scores = F.softmax(predicted_scores, dim=2)

        # Lists to store final predicted boxes, labels, and scores for all images
        all_images_boxes = []
        all_images_labels = []
        all_images_scores = []

        assert n_priors == predicted_locs.size(1) == predicted_scores.size(1)

        for i in range(batch_size):
            # Decode object coordinates from the form we regressed predicted boxes to
            decoded_locs = obj_detect_utils.cxcy_to_xy(
                obj_detect_utils.gcxgcy_to_cxcy(predicted_locs[i], self.priors_cxcy))

            # Lists to store boxes and scores for this image
            image_boxes = []
            image_labels = []
            image_scores = []

            # Check for each class
            for c in range(1, self.n_classes):
                # Keep only predicted boxes and scores where scores for this class are above the
                # minimum score
                class_scores = predicted_scores[i][:, c]
                score_above_min_score = class_scores > min_score
                n_above_min_score = score_above_min_score.sum().item()
                if n_above_min_score == 0:
                    continue
                class_scores = class_scores[score_above_min_score]
                class_decoded_locs = decoded_locs[score_above_min_score]  # (n_qualified, 4)

                # Sort predicted boxes and scores by scores
                class_scores, sort_ind = class_scores.sort(dim=0, descending=True)
                # (n_qualified), (n_min_score)
                class_decoded_locs = class_decoded_locs[sort_ind]  # (n_min_score, 4)

                # Find the overlap between predicted boxes
                overlap = obj_detect_utils.find_jaccard_overlap(class_decoded_locs,
                                                                class_decoded_locs)
                #print("overlap",overlap)
                # (n_qualified, n_min_score)

                # Non-Maximum Suppression (NMS)

                # A torch.bool tensor to keep track of which predicted boxes to suppress
                # True implies suppress, False implies don't suppress
                suppress = torch.zeros((n_above_min_score), dtype=torch.bool).to(self.device)
                # (n_qualified)

                # Consider each box in order of decreasing scores
                for box in range(class_decoded_locs.size(0)):
                    # If this box is already marked for suppression
                    if suppress[box]:
                        continue

                    # Suppress boxes whose overlaps (with this box) are greater than maximum
                    # overlap
                    # Find such boxes and update suppress indices
                    suppress = torch.logical_or(suppress, overlap[box] > max_overlap)
                    # The max operation retains previously suppressed boxes, like an 'OR' operation

                    # Don't suppress this box, even though it has an overlap of 1 with itself
                    suppress[box] = False

                # Store only unsuppressed boxes for this class
                image_boxes.append(class_decoded_locs[~suppress])
                image_labels.append(
                    torch.LongTensor((~suppress).sum().item() * [c]).to(self.device))
                image_scores.append(class_scores[~suppress])

            # If no object in any class is found, store a placeholder for 'background'
            if len(image_boxes) == 0:
                image_boxes.append(torch.FloatTensor([[0., 0., 1., 1.]]).to(self.device))
                image_labels.append(torch.LongTensor([0]).to(self.device))
                image_scores.append(torch.FloatTensor([0.]).to(self.device))

            # Concatenate into single tensors
            image_boxes = torch.cat(image_boxes, dim=0)  # (n_objects, 4)
            image_labels = torch.cat(image_labels, dim=0)  # (n_objects)
            image_scores = torch.cat(image_scores, dim=0)  # (n_objects)
            n_objects = image_scores.size(0)

            # Keep only the top k objects
            if n_objects > top_k:
                image_scores, sort_ind = image_scores.sort(dim=0, descending=True)
                image_scores = image_scores[:top_k]  # (top_k)
                image_boxes = image_boxes[sort_ind][:top_k]  # (top_k, 4)
                image_labels = image_labels[sort_ind][:top_k]  # (top_k)

            # Append to lists that store predicted boxes and scores for all images
            all_images_boxes.append(image_boxes)
            all_images_labels.append(image_labels)
            all_images_scores.append(image_scores)

        return all_images_boxes, all_images_labels, all_images_scores  # lists of length batch_size


def ai85tinierssdface(pretrained=False, **kwargs):
    """
    Constructs a Tinier SSD model
    """
    assert not pretrained
    return TinierSSDFace(aspect_ratios=TinierSSDFace.default_aspect_ratios, **kwargs)


models = [
    {
        'name': 'ai85tinierssdface',
        'min_input': 1,
        'dim': 2,
    }
]
