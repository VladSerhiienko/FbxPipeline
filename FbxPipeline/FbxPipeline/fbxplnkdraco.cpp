// draco_attributes_sources
#include "draco/attributes/attribute_octahedron_transform.cc"
#include "draco/attributes/attribute_quantization_transform.cc"
#include "draco/attributes/attribute_transform.cc"
#include "draco/attributes/geometry_attribute.cc"
#include "draco/attributes/point_attribute.cc"

// draco_compression_attributes_dec_sources
#include "draco/compression/attributes/attributes_decoder.cc"
#include "draco/compression/attributes/kd_tree_attributes_decoder.cc"
#include "draco/compression/attributes/sequential_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_attribute_decoders_controller.cc"
#include "draco/compression/attributes/sequential_integer_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_normal_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_quantization_attribute_decoder.cc"

// draco_compression_attributes_enc_sources
#include "draco/compression/attributes/attributes_encoder.cc"
#include "draco/compression/attributes/kd_tree_attributes_encoder.cc"
#include "draco/compression/attributes/sequential_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_attribute_encoders_controller.cc"
#include "draco/compression/attributes/sequential_integer_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_normal_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_quantization_attribute_encoder.cc"

// draco_compression_attributes_pred_schemes_enc_sources
#include "draco/compression/attributes/prediction_schemes/prediction_scheme_encoder_factory.cc"

// draco_compression_bit_coders_sources
#include "draco/compression/bit_coders/adaptive_rans_bit_decoder.cc"
#include "draco/compression/bit_coders/adaptive_rans_bit_encoder.cc"
#include "draco/compression/bit_coders/direct_bit_decoder.cc"
#include "draco/compression/bit_coders/direct_bit_encoder.cc"
#include "draco/compression/bit_coders/rans_bit_decoder.cc"
#include "draco/compression/bit_coders/rans_bit_encoder.cc"
#include "draco/compression/bit_coders/symbol_bit_decoder.cc"
#include "draco/compression/bit_coders/symbol_bit_encoder.cc"

// draco_compression_decode_sources
#include "draco/compression/decode.cc"

// draco_compression_encode_sources
#include "draco/compression/encode.cc"
#include "draco/compression/expert_encode.cc"

// draco_compression_mesh_dec_sources
#include "draco/compression/mesh/mesh_decoder.cc"
#include "draco/compression/mesh/mesh_edgebreaker_decoder.cc"
#include "draco/compression/mesh/mesh_edgebreaker_decoder_impl.cc"
#include "draco/compression/mesh/mesh_sequential_decoder.cc"

// draco_compression_mesh_enc_sources
#include "draco/compression/mesh/mesh_edgebreaker_encoder.cc"
#include "draco/compression/mesh/mesh_edgebreaker_encoder_impl.cc"
#include "draco/compression/mesh/mesh_encoder.cc"
#include "draco/compression/mesh/mesh_sequential_encoder.cc"

// draco_compression_point_cloud_dec_sources
#include "draco/compression/point_cloud/point_cloud_decoder.cc"
#include "draco/compression/point_cloud/point_cloud_kd_tree_decoder.cc"
#include "draco/compression/point_cloud/point_cloud_sequential_decoder.cc"

// draco_compression_point_cloud_enc_sources
#include "draco/compression/point_cloud/point_cloud_encoder.cc"
#include "draco/compression/point_cloud/point_cloud_kd_tree_encoder.cc"
#include "draco/compression/point_cloud/point_cloud_sequential_encoder.cc"

// draco_compression_entropy_sources
#include "draco/compression/entropy/shannon_entropy.cc"
#include "draco/compression/entropy/symbol_decoding.cc"
#include "draco/compression/entropy/symbol_encoding.cc"

// draco_core_sources
#include "draco/core/bit_utils.cc"
#include "draco/core/bounding_box.cc"
#include "draco/core/cycle_timer.cc"
#include "draco/core/data_buffer.cc"
#include "draco/core/decoder_buffer.cc"
#include "draco/core/divide.cc"
#include "draco/core/draco_types.cc"
#include "draco/core/encoder_buffer.cc"
#include "draco/core/hash_utils.cc"
#include "draco/core/options.cc"
#include "draco/core/quantization_utils.cc"

// draco_mesh_sources
#include "draco/mesh/corner_table.cc"
#include "draco/mesh/mesh.cc"
#include "draco/mesh/mesh_are_equivalent.cc"
#include "draco/mesh/mesh_attribute_corner_table.cc"
#include "draco/mesh/mesh_cleanup.cc"
#include "draco/mesh/mesh_misc_functions.cc"
#include "draco/mesh/mesh_stripifier.cc"
#include "draco/mesh/triangle_soup_mesh_builder.cc"

// draco_point_cloud_sources
#include "draco/point_cloud/point_cloud.cc"
#include "draco/point_cloud/point_cloud_builder.cc"

// draco_points_dec_sources
#include "draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_decoder.cc"
#include "draco/compression/point_cloud/algorithms/float_points_tree_decoder.cc"

// draco_points_enc_sources
#include "draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_encoder.cc"
#include "draco/compression/point_cloud/algorithms/float_points_tree_encoder.cc"

#include "draco/metadata/geometry_metadata.cc"
#include "draco/metadata/metadata.cc"
#include "draco/metadata/metadata_decoder.cc"
#include "draco/metadata/metadata_encoder.cc"