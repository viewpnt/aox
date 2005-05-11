// Copyright Oryx Mail Systems GmbH. All enquiries to info@oryx.com, please.

#ifndef CREATE_H
#define CREATE_H

#include "command.h"


class Create
    : public Command
{
public:
    Create();

    void parse();
    void execute();

private:
    class CreateData * d;
};


#endif
