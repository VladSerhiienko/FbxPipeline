#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include <tr1/functional>

#include <ply.hpp>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

using namespace std::tr1::placeholders;

class ply_to_obj_converter
{
public:
  typedef int flags_type;
  typedef enum { triangulate = 1 << 0 };
  ply_to_obj_converter(flags_type flags = 0);
  bool convert(std::istream& istream, const std::string& istream_filename, std::ostream& ostream, const std::string& ostream_filename);
private:
  void info_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  void warning_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  void error_callback(const std::string& filename, std::size_t line_number, const std::string& message);
  std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> > element_definition_callback(const std::string& element_name, std::size_t count);
  template <typename ScalarType> std::tr1::function<void (ScalarType)> scalar_property_definition_callback(const std::string& element_name, const std::string& property_name);
  template <typename SizeType, typename ScalarType> std::tr1::tuple<std::tr1::function<void (SizeType)>, std::tr1::function<void (ScalarType)>, std::tr1::function<void ()> > list_property_definition_callback(const std::string& element_name, const std::string& property_name);
  void vertex_begin();
  void vertex_x(ply::float32 x);
  void vertex_y(ply::float32 y);
  void vertex_z(ply::float32 z);
  void vertex_end();
  void face_begin();
  void face_vertex_indices_begin(ply::uint8 size);
  void face_vertex_indices_element(ply::int32 vertex_index);
  void face_vertex_indices_end();
  void face_end();
  flags_type flags_;
  std::ostream* ostream_;
  double vertex_x_, vertex_y_, vertex_z_;
  std::size_t face_vertex_indices_element_index_, face_vertex_indices_first_element_, face_vertex_indices_previous_element_;
};

ply_to_obj_converter::ply_to_obj_converter(flags_type flags)
  : flags_(flags)
{
}

void ply_to_obj_converter::info_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "info: " << message << std::endl;
}

void ply_to_obj_converter::warning_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "warning: " << message << std::endl;
}

void ply_to_obj_converter::error_callback(const std::string& filename, std::size_t line_number, const std::string& message)
{
  std::cerr << filename << ":" << line_number << ": " << "error: " << message << std::endl;
}

std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> > ply_to_obj_converter::element_definition_callback(const std::string& element_name, std::size_t count)
{
  if (element_name == "vertex") {
    return std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> >(
      std::tr1::bind(&ply_to_obj_converter::vertex_begin, this),
      std::tr1::bind(&ply_to_obj_converter::vertex_end, this)
    );
  }
  else if (element_name == "face") {
    return std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> >(
      std::tr1::bind(&ply_to_obj_converter::face_begin, this),
      std::tr1::bind(&ply_to_obj_converter::face_end, this)
    );
  }
  else {
    return std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> >(0, 0);
  }
}

template <>
std::tr1::function<void (ply::float32)> ply_to_obj_converter::scalar_property_definition_callback(const std::string& element_name, const std::string& property_name)
{
  if (element_name == "vertex") {
    if (property_name == "x") {
      return std::tr1::bind(&ply_to_obj_converter::vertex_x, this, _1);
    }
    else if (property_name == "y") {
      return std::tr1::bind(&ply_to_obj_converter::vertex_y, this, _1);
    }
    else if (property_name == "z") {
      return std::tr1::bind(&ply_to_obj_converter::vertex_z, this, _1);
    }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

template <>
std::tr1::tuple<std::tr1::function<void (ply::uint8)>, std::tr1::function<void (ply::int32)>, std::tr1::function<void ()> > ply_to_obj_converter::list_property_definition_callback(const std::string& element_name, const std::string& property_name)
{
  if ((element_name == "face") && (property_name == "vertex_indices")) {
    return std::tr1::tuple<std::tr1::function<void (ply::uint8)>, std::tr1::function<void (ply::int32)>, std::tr1::function<void ()> >(
      std::tr1::bind(&ply_to_obj_converter::face_vertex_indices_begin, this, _1),
      std::tr1::bind(&ply_to_obj_converter::face_vertex_indices_element, this, _1),
      std::tr1::bind(&ply_to_obj_converter::face_vertex_indices_end, this)
    );
  }
  else {
    return std::tr1::tuple<std::tr1::function<void (ply::uint8)>, std::tr1::function<void (ply::int32)>, std::tr1::function<void ()> >(0, 0, 0);
  }
}

void ply_to_obj_converter::vertex_begin()
{
}

void ply_to_obj_converter::vertex_x(ply::float32 x)
{
  vertex_x_ = x;
}

void ply_to_obj_converter::vertex_y(ply::float32 y)
{
  vertex_y_ = y;
}

void ply_to_obj_converter::vertex_z(ply::float32 z)
{
  vertex_z_ = z;
}

void ply_to_obj_converter::vertex_end()
{
  (*ostream_) << "v " << vertex_x_ << " " << vertex_y_ << " " << vertex_z_ << "\n";
}

void ply_to_obj_converter::face_begin()
{
  if (!(flags_ & triangulate)) {
    (*ostream_) << "f";
  }
}

void ply_to_obj_converter::face_vertex_indices_begin(ply::uint8 size)
{
  face_vertex_indices_element_index_ = 0;
}

void ply_to_obj_converter::face_vertex_indices_element(ply::int32 vertex_index)
{
  if (flags_ & triangulate) {
    if (face_vertex_indices_element_index_ == 0) {
      face_vertex_indices_first_element_ = vertex_index;
    }
    else if (face_vertex_indices_element_index_ == 1) {
      face_vertex_indices_previous_element_ = vertex_index;
    }
    else {
      (*ostream_) << "f " << (face_vertex_indices_first_element_ + 1) << " " << (face_vertex_indices_previous_element_ + 1) << " " << (vertex_index + 1) << "\n";
      face_vertex_indices_previous_element_ = vertex_index;
    }
    ++face_vertex_indices_element_index_;
  }
  else {
    (*ostream_) << " " << (vertex_index + 1);
  }
}

void ply_to_obj_converter::face_vertex_indices_end()
{
  if (!(flags_ & triangulate)) {
    (*ostream_) << "\n";
  }
}

void ply_to_obj_converter::face_end()
{
}

bool ply_to_obj_converter::convert(std::istream& istream, const std::string& istream_filename, std::ostream& ostream, const std::string& ostream_filename)
{
  ply::ply_parser::flags_type ply_parser_flags = 0;
  ply::ply_parser ply_parser(ply_parser_flags);

  ply_parser.info_callback(std::tr1::bind(&ply_to_obj_converter::info_callback, this, std::tr1::ref(istream_filename), _1, _2));
  ply_parser.warning_callback(std::tr1::bind(&ply_to_obj_converter::warning_callback, this, std::tr1::ref(istream_filename), _1, _2));
  ply_parser.error_callback(std::tr1::bind(&ply_to_obj_converter::error_callback, this, std::tr1::ref(istream_filename), _1, _2)); 

  ply_parser.element_definition_callback(std::tr1::bind(&ply_to_obj_converter::element_definition_callback, this, _1, _2));

  ply::ply_parser::scalar_property_definition_callbacks_type scalar_property_definition_callbacks;
  ply::at<ply::float32>(scalar_property_definition_callbacks) = std::tr1::bind(&ply_to_obj_converter::scalar_property_definition_callback<ply::float32>, this, _1, _2);
  ply_parser.scalar_property_definition_callbacks(scalar_property_definition_callbacks);

  ply::ply_parser::list_property_definition_callbacks_type list_property_definition_callbacks;
  ply::at<ply::uint8, ply::int32>(list_property_definition_callbacks) = std::tr1::bind(&ply_to_obj_converter::list_property_definition_callback<ply::uint8, ply::int32>, this, _1, _2);
  ply_parser.list_property_definition_callbacks(list_property_definition_callbacks);

  ostream_ = &ostream;

  return ply_parser.parse(istream);
}

int main(int argc, char* argv[])
{
  ply_to_obj_converter::flags_type ply_to_obj_converter_flags = 0;

  int argi;
  for (argi = 1; argi < argc; ++argi) {

    if (argv[argi][0] != '-') {
      break;
    }
    if (argv[argi][1] == 0) {
      ++argi;
      break;
    }
    char short_opt, *long_opt, *opt_arg;
    if (argv[argi][1] != '-') {
      short_opt = argv[argi][1];
      opt_arg = &argv[argi][2];
      long_opt = &argv[argi][2];
      while (*long_opt != '\0') {
        ++long_opt;
      }
    }
    else {
      short_opt = 0;
      long_opt = &argv[argi][2];
      opt_arg = long_opt;
      while ((*opt_arg != '=') && (*opt_arg != '\0')) {
        ++opt_arg;
      }
      if (*opt_arg == '=') {
        *opt_arg++ = '\0';
      }
    }

    if ((short_opt == 'h') || (std::strcmp(long_opt, "help") == 0)) {
      std::cout << "Usage: ply2obj [OPTION] [[INFILE] OUTFILE]\n";
      std::cout << "Convert a PLY file to an OBJ file.\n";
      std::cout << "\n";
      std::cout << "  -h, --help       display this help and exit\n";
      std::cout << "  -v, --version    output version information and exit\n";
      std::cout << "  -f, --flag=FLAG  set flag\n";
      std::cout << "\n";
      std::cout << "FLAG may be one of the following: triangulate.\n";
      std::cout << "\n";
      std::cout << "With no INFILE/OUTFILE, or when INFILE/OUTFILE is -, read standard input/output.\n";
      std::cout << "\n";
      std::cout << "The following PLY elements and properties are supported.\n";
      std::cout << "  element vertex\n";
      std::cout << "    property float32 x\n";
      std::cout << "    property float32 y\n";
      std::cout << "    property float32 z\n";
      std::cout << "  element face\n";
      std::cout << "    property list uint8 int32 vertex_indices.\n";
      std::cout << "\n";
      std::cout << "Report bugs to <" << PACKAGE_BUGREPORT << ">.\n";
      return EXIT_SUCCESS;
    }

    else if ((short_opt == 'v') || (std::strcmp(long_opt, "version") == 0)) {
      std::cout << "ply2obj (" << PACKAGE_NAME << ") " << PACKAGE_VERSION << "\n";
      std::cout << "Copyright (C) 2007 " << PACKAGE_AUTHOR << "\n";
      std::cout << "\n";
      std::cout << "This program is free software; you can redistribute it and/or modify\n";
      std::cout << "it under the terms of the GNU General Public License as published by\n";
      std::cout << "the Free Software Foundation; either version 2 of the License, or\n";
      std::cout << "(at your option) any later version.\n";
      std::cout << "\n";
      std::cout << "This program is distributed in the hope that it will be useful,\n";
      std::cout << "but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
      std::cout << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n";
      std::cout << "GNU General Public License for more details.\n";
      std::cout << "\n";
      std::cout << "You should have received a copy of the GNU General Public License\n";
      std::cout << "along with this program; if not, write to the Free Software\n";
      std::cout << "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n";
      return EXIT_SUCCESS;
    }

    else if ((short_opt == 'f') || (std::strcmp(long_opt, "flag") == 0)) {
      if (strcmp(opt_arg, "triangulate") == 0) {
        ply_to_obj_converter_flags |= ply_to_obj_converter::triangulate;
      }
      else {
        std::cerr << "ply2obj : " << "invalid option `" << argv[argi] << "'" << "\n";
        std::cerr << "Try `" << argv[0] << " --help' for more information.\n";
        return EXIT_FAILURE;
      }
    }

    else {
      std::cerr << "ply2obj: " << "invalid option `" << argv[argi] << "'" << "\n";
      std::cerr << "Try `" << argv[0] << " --help' for more information.\n";
      return EXIT_FAILURE;
    }
  }

  int parc = argc - argi;
  char** parv = argv + argi;
  if (parc > 2) {
    std::cerr << "ply2obj: " << "too many parameters" << "\n";
    std::cerr << "Try `" << argv[0] << " --help' for more information.\n";
    return EXIT_FAILURE;
  }

  std::ifstream ifstream;
  const char* istream_filename = "";
  if (parc > 0) {
    istream_filename = parv[0];
    if (std::strcmp(istream_filename, "-") != 0) {
      ifstream.open(istream_filename);
      if (!ifstream.is_open()) {
        std::cerr << "ply2obj: " << istream_filename << ": " << "no such file or directory" << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  std::ofstream ofstream;
  const char* ostream_filename = "";
  if (parc > 1) {
    ostream_filename = parv[1];
    if (std::strcmp(ostream_filename, "-") != 0) {
      ofstream.open(ostream_filename);
      if (!ofstream.is_open()) {
        std::cerr << "ply2obj: " << ostream_filename << ": " << "could not open file" << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  std::istream& istream = ifstream.is_open() ? ifstream : std::cin;
  std::ostream& ostream = ofstream.is_open() ? ofstream : std::cout;

  class ply_to_obj_converter ply_to_obj_converter(ply_to_obj_converter_flags);
  return ply_to_obj_converter.convert(istream, istream_filename, ostream, ostream_filename);
}
