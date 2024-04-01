/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2023/06/28.
//

#include <sstream>
#include "sql/parser/value.h"
#include "storage/field/field.h"
#include "common/log/log.h"
#include "common/lang/comparator.h"
#include "common/lang/string.h"

const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "dates", "floats", "booleans"};

const char *attr_type_to_string(AttrType type)
{
  if (type >= UNDEFINED && type <= FLOATS) {
    return ATTR_TYPE_NAME[type];
  }
  return "unknown";
}
AttrType attr_type_from_string(const char *s)
{
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    if (0 == strcmp(ATTR_TYPE_NAME[i], s)) {
      return (AttrType)i;
    }
  }
  return UNDEFINED;
}

Value::Value(int val)
{
  set_int(val);
}

Value::Value(float val)
{
  set_float(val);
}

Value::Value(bool val)
{
  set_boolean(val);
}

Value::Value(const char *s, int len /*= 0*/)
{
  set_string(s, len);
}

bool Value::check(int val) const
{
  int year, mon, day;
  year = val / 10000;
  mon = (val - year * 10000) / 100;
  day = val - year * 10000 - mon * 100;
  if(year<1970||year>2038||mon<1||mon>12||day<1||day>31){
    return 0;
  }
  if(mon==4||mon==2||mon==6||mon==9||mon==11){
    if(day>30){
      return 0;
    }
    if(mon==2){
      if((year%400==0||(year%4==0))&&(year%100!=0)){
        if(day>29){
          return 0;
        }
      }
      else{
        if(day>28){
          return 0;
        }
      }
    }
  }
  return 1;
}

Value::Value(const char *date, int len, int flag)
{
  int val;
  //zhuan wei shu zi
  if(flag==1){
    if(len==10){
      val=(int(date[0])-48)*10000000+(int(date[1])-48)*1000000+(int(date[2])-48)*100000+(int(date[3])-48)*10000+(int(date[5])-48)*1000+(int(date[6])-48)*100+(int(date[8])-48)*10+int(date[9])-48;
    }
    else{
      if(len==9){
        if(date[6]=='-'){
          val=(int(date[0])-48)*10000000+(int(date[1])-48)*1000000+(int(date[2])-48)*100000+(int(date[3])-48)*10000+(int(date[5])-48)*100+(int(date[7])-48)*10+int(date[8])-48;
        }
        else{
          val=(int(date[0])-48)*10000000+(int(date[1])-48)*1000000+(int(date[2])-48)*100000+(int(date[3])-48)*10000+(int(date[5])-48)*1000+(int(date[6])-48)*100+int(date[8])-48;
        }
      }
      else{
        val=(int(date[0])-48)*10000000+(int(date[1])-48)*1000000+(int(date[2])-48)*100000+(int(date[3])-48)*10000+(int(date[5])-48)*100+int(date[7])-48;
      }
    }
  } 
  bool check_flag = check(val);
  if(check_flag==1){
    set_date(val);
  }
  else{
    throw("wrongdate");
    //ASSERT(false, "got an invalid value type");//shu ru bu he fa
  }
}

void Value::set_data(char *data, int length)
{
  switch (attr_type_) {
    case CHARS: {
      set_string(data, length);
    } break;
    case INTS: {
      num_value_.int_value_ = *(int *)data;
      length_ = length;
    } break;
    case FLOATS: {
      num_value_.float_value_ = *(float *)data;
      length_ = length;
    } break;
    case BOOLEANS: {
      num_value_.bool_value_ = *(int *)data != 0;
      length_ = length;
    } break;
    case DATES: {
      int f=*(int *)data;
      if(check(f)==1){
        num_value_.date_value_ = *(int *)data;
        length_ = length;
      }
      else{
        ASSERT(false, "got an invalid value type");//shu ru bu he fa
      }
    } break;
    default: {
      LOG_WARN("unknown data type: %d", attr_type_);
    } break;
  }
}
void Value::set_int(int val)
{
  attr_type_ = INTS;
  num_value_.int_value_ = val;
  length_ = sizeof(val);
}

void Value::set_float(float val)
{
  attr_type_ = FLOATS;
  num_value_.float_value_ = val;
  length_ = sizeof(val);
}
void Value::set_boolean(bool val)
{
  attr_type_ = BOOLEANS;
  num_value_.bool_value_ = val;
  length_ = sizeof(val);
}
void Value::set_string(const char *s, int len /*= 0*/)
{
  attr_type_ = CHARS;
  if (len > 0) {
    len = strnlen(s, len);
    str_value_.assign(s, len);
  } else {
    str_value_.assign(s);
  }
  length_ = str_value_.length();
}

void Value::set_date(int val)
{
  attr_type_ = DATES;
  num_value_.date_value_ = val;
  length_ = sizeof(val);
}

void Value::set_value(const Value &value)
{
  switch (value.attr_type_) {
    case INTS: {
      set_int(value.get_int());
    } break;
    case FLOATS: {
      set_float(value.get_float());
    } break;
    case CHARS: {
      set_string(value.get_string().c_str());
    } break;
    case BOOLEANS: {
      set_boolean(value.get_boolean());
    } break;
    case DATES: {
      set_date(value.get_date());
      break;
    }
    case UNDEFINED: {
      ASSERT(false, "got an invalid value type");
    } break;
  }
}

const char *Value::data() const
{
  switch (attr_type_) {
    case CHARS: {
      return str_value_.c_str();
    } break;
    default: {
      return (const char *)&num_value_;
    } break;
  }
}

std::string Value::to_string() const
{
  std::stringstream os;
  switch (attr_type_) {
    case INTS: {
      os << num_value_.int_value_;
    } break;
    case FLOATS: {
      os << common::double_to_str(num_value_.float_value_);
    } break;
    case BOOLEANS: {
      os << num_value_.bool_value_;
    } break;
    case CHARS: {
      os << str_value_;
    } break;
    case DATES: {
      std::string result;
      result=int_to_str(num_value_.date_value_, 10);
      os << result;
    } break;
    default: {
      LOG_WARN("unsupported attr type: %d", attr_type_);
    } break;
  }
  return os.str();
}

int Value::str_to_int(const char* date)
{
  int len=strlen(date);
  int val;
  val=(int(date[0])-48)*10000000+(int(date[1])-48)*1000000+(int(date[2])-48)*100000+(int(date[3])-48)*10000+(int(date[4])-48)*1000+(int(date[5])-48)*100+(int(date[6])-48)*10+int(date[7])-48;
  return val;
}

char* Value::int_to_str(int val, int len) const
{
  int year, mon, day;
  year = val / 10000;
  mon = (val - year * 10000) / 100;
  day = val - year * 10000 - mon * 100;
  char *res=new char[11];
  res[0]=char(year/1000+48);
  int y2=year%1000/100;
  if(y2==0){
    res[1]='0';
  }
  else{
    res[1]=char(year%1000/100+48);
  }
  int y3=year%100/10;
  if(y3==0){
    res[2]='0';
  }
  else{
    res[2]=char(year%100/10+48);
  }
  int y4=year%10;
  if(y4==0){
    res[3]='0';
  }
  else{
    res[3]=char(year%10+48);
  }
  res[4]='-';
  int m1=mon/10;
  if(m1==0){
    res[5]='0';
  }
  else{
    res[5]=char(mon/10+48);
  }
  int m2=mon%10;
  if(m2==0){
    res[6]='0';
  }
  else{
    res[6]=char(mon%10+48);
  }
  res[7]='-';
  int d1=day/10;
  if(d1==0){
    res[8]='0';
  }
  else{
    res[8]=char(day/10+48);
  }
  int d2=day%10;
  if(d2==0){
    res[9]='0';
  }
  else{
    res[9]=char(day%10+48);
  }
  res[10]='\0';
  return res;
}

int Value::compare(const Value &other) const
{
  if (this->attr_type_ == other.attr_type_) {
    switch (this->attr_type_) {
      case INTS: {
        return common::compare_int((void *)&this->num_value_.int_value_, (void *)&other.num_value_.int_value_);//common::
      } break;
      case FLOATS: {
        return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other.num_value_.float_value_);//common::
      } break;
      case CHARS: {
        return common::compare_string((void *)this->str_value_.c_str(),//common::
            this->str_value_.length(),
            (void *)other.str_value_.c_str(),
            other.str_value_.length());
      } break;
      case BOOLEANS: {
        return common::compare_int((void *)&this->num_value_.bool_value_, (void *)&other.num_value_.bool_value_);//common::
      }
      case DATES: {
        return common::compare_int((void *)&this->num_value_.date_value_, (void *)&other.num_value_.date_value_);//common::
      } break;
      default: {
        LOG_WARN("unsupported type: %d", this->attr_type_);
      }
    }
  } else if (this->attr_type_ == INTS && other.attr_type_ == FLOATS) {
    float this_data = this->num_value_.int_value_;
    return common::compare_float((void *)&this_data, (void *)&other.num_value_.float_value_);//common::
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == INTS) {
    float other_data = other.num_value_.int_value_;
    return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other_data);//common::
  }
  LOG_WARN("not supported");
  return -1;  // TODO return rc?
}

int Value::get_int() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return (int)(std::stol(str_value_));
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0;
      }
    }
    case INTS: {
      return num_value_.int_value_;
    }
    case FLOATS: {
      return (int)(num_value_.float_value_);
    }
    case BOOLEANS: {
      return (int)(num_value_.bool_value_);
    }
    case DATES: {
      return (int)(num_value_.date_value_);
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

float Value::get_float() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return std::stof(str_value_);
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0.0;
      }
    } break;
    case INTS: {
      return float(num_value_.int_value_);
    } break;
    case FLOATS: {
      return num_value_.float_value_;
    } break;
    case BOOLEANS: {
      return float(num_value_.bool_value_);
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

std::string Value::get_string() const
{
  return this->to_string();
}

int Value::get_date() const
{
  switch (attr_type_) {
    case DATES: {
      return num_value_.date_value_;
    }
    case INTS: {
      return num_value_.int_value_;
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

bool Value::get_boolean() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        float val = std::stof(str_value_);
        if (val >= EPSILON || val <= -EPSILON) {
          return true;
        }

        int int_val = std::stol(str_value_);
        if (int_val != 0) {
          return true;
        }

        return !str_value_.empty();
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float or integer. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return !str_value_.empty();
      }
    } break;
    case INTS: {
      return num_value_.int_value_ != 0;
    } break;
    case FLOATS: {
      float val = num_value_.float_value_;
      return val >= EPSILON || val <= -EPSILON;
    } break;
    case BOOLEANS: {
      return num_value_.bool_value_;
    } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return false;
    }
  }
  return false;
}
