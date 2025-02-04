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
Script to generate Face Id embeddings
"""
import argparse
import os.path as path
import numpy as np
import torch
from ai85.ai85_adapter import AI85SimulatorAdapter
from ai85.ai85_facedet_adapter import Facedet_AI85SimulatorAdapter

from utils import append_db_file_from_path, save_embedding_db, create_embeddings_include_file

CURRENT_DIR = path.abspath(path.dirname(path.abspath(__file__)))
MODEL_PATH = path.join(CURRENT_DIR, 'model', 'ai85_faceid_aug_qat_best-q.pth.tar')
FACEDET_PATH = path.join(CURRENT_DIR, 'model', 'facedet_qat_best.pth.tar')


def create_db_from_folder(args):
    """
    Main function of the script to generate face detector, AI85 simulator and calls the utility
    functions to generate embeddings and store them in required format.
    """


    ai85_adapter = AI85SimulatorAdapter(MODEL_PATH)
    face_detector = Facedet_AI85SimulatorAdapter(FACEDET_PATH)

    embedding_db, _ = append_db_file_from_path(args.db, face_detector, ai85_adapter,
                                               db_dict=None, verbose=True)
    if not embedding_db:
        print(f'Cannot create a DB file. No face could be detected from the images in folder ',
              f'`{args.db}`')
        return

    save_embedding_db(embedding_db, path.join(CURRENT_DIR, args.db_filename + '.bin'),
                      add_prev_imgs=True)
    create_embeddings_include_file(CURRENT_DIR, args.db_filename, args.include_path)


def parse_arguments():
    """
    Function to parse comman line arguments.
    """
    parser = argparse.ArgumentParser(description='Create embedding database file.')
    parser.add_argument('--db', '-db-path', type=str, default='db',
                        help='path for face images')
    parser.add_argument('--db-filename', type=str, default='embeddings',
                        help='filename to store embeddings')
    parser.add_argument('--include-path', type=str, default='embeddings',
                        help='path to include folder')

    args = parser.parse_args()
    return args


def main():
    """
    Entry point of the script to parse command line arguments and run the function to generate
    embeddings.
    """
    # make deterministic
    torch.manual_seed(0)
    np.random.seed(0)
    
    args = parse_arguments()
    create_db_from_folder(args)


if __name__ == "__main__":
    main()
