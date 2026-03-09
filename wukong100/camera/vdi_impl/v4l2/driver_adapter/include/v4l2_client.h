/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SPRDCAMERA_V4L2_HANDLER_H__
#define __SPRDCAMERA_V4L2_HANDLER_H__
#include <poll.h>
#include <stdio.h>

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct sensor_reg_info {
  int reg_addr;
  int reg_value;
};

class V4l2AdapterInterface {
 public:
  virtual ~V4l2AdapterInterface(){};
  virtual int open(const char *path, int flags) = 0;
  virtual long ioctl(int fd_, unsigned long request, void *argp) = 0;
  virtual void *mmap(void *addr, size_t length, int prot, int flags, int fd,
                     off_t offset) = 0;
  virtual int munmap(void *addr, size_t length) = 0;
  virtual int poll(struct pollfd *fds, nfds_t nfds, int timeout) = 0;
  virtual int close(int flags) = 0;
  virtual int write_i2c_t(int fd_, void *reg, int count) {
    return this->write_i2c_t(fd_, reg, count);
  };
  virtual int read_i2c(int fd_, unsigned short reg_addr) {
    return this->read_i2c(fd_, reg_addr);
  };

  template <class T, int N>
  int write_i2c(int fd_, T (&array)[N]) {
    int count = 0;
    for (int i = 0; i < N; i++) {
      if (!array) break;
      if (!((struct sensor_reg_info)array[i]).reg_addr &&
          !((struct sensor_reg_info)array[i]).reg_value)
        continue;
      count++;
    }
    return write_i2c_t(fd_, (void *)array, count);
  };
  const char *name_;
};

class V4l2Client {
 public:
  V4l2Client(const char *name);
  virtual ~V4l2Client(){};

  static V4l2AdapterInterface *create();
  static void registerType(V4l2Client *factory);
  const std::string &name() const { return name_; }

 private:
  virtual V4l2AdapterInterface *createInstance();
  std::string name_;
};

class UnisocV4l2ClientFactory : public V4l2Client {
 public:
  UnisocV4l2ClientFactory() : V4l2Client("UnisocV4l2Client"){};
};
#endif /* __LIBCAMERA_PIPELINE_HANDLER_H__ */
