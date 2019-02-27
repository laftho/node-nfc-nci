/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TAGMANAGER_H
#define TAGMANAGER_H

#include <string>
#include <exception>
#include <linux_nfc_api.h>

class TagTechnology
{
public:
  std::string name;
  std::string type;
  unsigned int code;
};

class TagUid
{
public:
  std::string id;
  std::string type;
  unsigned int length;
};

class TagNDEF
{
public:
  TagNDEF();
  unsigned int size;
  unsigned int length;
  bool writeable;
  std::string type;
  std::string content;
};

class Tag
{
public:
  TagTechnology technology;
  TagUid uid;
  TagNDEF ndef;
};

class ITagManager {
public:
  virtual void onTag(Tag tag) = 0;
};

/**
 * @todo write docs
 */
class TagManager
{
private:
  ITagManager* tagInterface;
  
  TagManager();
  
  TagManager(TagManager const&);
  void operator=(TagManager const&);
public:
    /**
     * Default constructor
     */
    

    /**
     * Destructor
     */
    ~TagManager();

    void initialize(ITagManager& tagInterface);

    void onTagArrival(nfc_tag_info_t *pTagInfo);
    void onTagDeparture(void);
    
    void onDeviceArrival(void);
    void onDeviceDeparture(void);
    void onMessageReceived(unsigned char *message, unsigned int length);
    
    void onSnepClientReady();
    void onSnepClientClosed();
    
    static TagManager& getInstance();
};

#endif // TAGMANAGER_H
