#pragma once

#include <bsoncxx/builder/basic/document.hpp>
#include "coco_parameter.hpp"

namespace coco
{
  /**
   * @brief The bson_parameter_converter class is an abstract base class for converting coco_parameter objects to BSON documents and vice versa.
   */
  class bson_parameter_converter
  {
  public:
    /**
     * @brief Constructs a bson_parameter_converter object with the specified type.
     *
     * @param type The type of the converter.
     */
    bson_parameter_converter(const std::string &type) : type(type) {}

    /**
     * @brief Destructor for the bson_parameter_converter class.
     */
    virtual ~bson_parameter_converter() = default;

    /**
     * @brief Returns the type of the converter.
     *
     * @return The type of the converter.
     */
    [[nodiscard]] const std::string &get_type() const { return type; }

    /**
     * @brief Converts a coco_parameter object to a BSON document.
     *
     * @param p The coco_parameter object to convert.
     * @return The BSON document representing the coco_parameter object.
     */
    virtual bsoncxx::builder::basic::document to_bson(const coco_parameter &p) const = 0;

    /**
     * @brief Converts a BSON document to a coco_parameter object.
     *
     * @param doc The BSON document to convert.
     * @return A unique pointer to the coco_parameter object.
     */
    virtual std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const = 0;

  protected:
    const std::string type; // The type of the converter.
  };

  class integer_parameter_converter : public bson_parameter_converter
  {
  public:
    integer_parameter_converter() : bson_parameter_converter("integer") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };

  class real_parameter_converter : public bson_parameter_converter
  {
  public:
    real_parameter_converter() : bson_parameter_converter("real") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };

  class boolean_parameter_converter : public bson_parameter_converter
  {
  public:
    boolean_parameter_converter() : bson_parameter_converter("boolean") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };

  class symbolic_parameter_converter : public bson_parameter_converter
  {
  public:
    symbolic_parameter_converter() : bson_parameter_converter("symbol") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };

  class string_parameter_converter : public bson_parameter_converter
  {
  public:
    string_parameter_converter() : bson_parameter_converter("string") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };

  class array_parameter_converter : public bson_parameter_converter
  {
  public:
    array_parameter_converter() : bson_parameter_converter("array") {}

    bsoncxx::builder::basic::document to_bson(const coco_parameter &t) const override;
    std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const override;
  };
} // namespace coco