#pragma once

namespace coco
{
  class coco;

  class coco_db
  {
  public:
    coco_db(coco &cc);
    ~coco_db();

  protected:
    coco &cc;
  };
} // namespace coco
