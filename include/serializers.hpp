#ifndef __VCPU_SERIALIZER_HPP__
#define __VCPU_SERIALIZER_HPP__

#include <iostream>
#include <string>
#include <string.h>
#include <format.hpp>
#include <vector>
#include <libvirt/libvirt.h>

namespace serializer
{
    inline void vcpu_metrics(std::string &out, size_t rc, virDomainStatsRecordPtr *stats)
    {
        for (size_t j = 0; j < rc; j++)
        {
            virDomainStatsRecordPtr record = stats[j];

            for (int k = 0; k < record->nparams; k++)
            {

                char tenantuuid[4096] = {0};
                custom::virDomainGetTenant(record->dom, &tenantuuid[0]);

                char uuid[4096] = {0};
                virDomainGetUUIDString(record->dom, uuid);

                // <instance>
                // <tenant>
                // <uuid>43dc0cf8-809b-4adb-9bea-a9abb5f3d90e</uuid>
                // </tenant>
                // </instance>

                // input is vcpu, id, param
                std::vector<std::string> fields = custom::split(record->params[k].field);
                if (fields.size() == 3)
                {
                    out.append(custom::format("# TYPE libvirt_%s_%s counter\n", fields[0].c_str(), fields[2].c_str()));
                    out.append(custom::format("libvirt_%s_%s{domain=\"%s\", vcpu=\"%s\" uuid=\"%s\" tenant=\"%s\"} %llu\n",
                                              fields[0].c_str(),
                                              fields[2].c_str(),
                                              virDomainGetName(record->dom),
                                              fields[1].c_str(),
                                              uuid,
                                              tenantuuid,
                                              record->params[k].value.ul));
                }
            }
        }
    }

    inline void network_metrics(std::string &out, size_t rc, virDomainStatsRecordPtr *stats)
    {
        for (size_t j = 0; j < rc; j++)
        {
            virDomainStatsRecordPtr record = stats[j];
            std::string netname;

            char tenantuuid[4096] = {0};
            custom::virDomainGetTenant(record->dom, &tenantuuid[0]);

            char uuid[4096] = {0};
            virDomainGetUUIDString(record->dom, uuid);

            for (int k = 0; k < record->nparams; k++)
            {

                // input is vcpu, id, param
                std::vector<std::string> fields = custom::split(record->params[k].field);
                if (fields.size() == 3)
                {
                    netname = record->params[k].value.s;
                }

                if (fields.size() == 4)
                {
                    out.append(custom::format("# TYPE libvirt_%s_%s_%s counter\n", fields[0].c_str(), fields[3].c_str(), fields[2].c_str()));
                    out.append(custom::format("libvirt_%s_%s_%s{domain=\"%s\" interfaceid=\"%s\", name=\"%s\" uuid=\"%s\" tenant=\"%s\"} %llu\n",
                                              fields[0].c_str(), fields[3].c_str(), fields[2].c_str(), // => net_bytes_rx
                                              virDomainGetName(record->dom), fields[1].c_str(), netname.c_str(),
                                              uuid,
                                              tenantuuid,
                                              record->params[k].value.ul));
                }
            }
        }
    }

    inline void block_metrics(std::string &out, size_t rc, virDomainStatsRecordPtr *stats)
    {
        for (size_t j = 0; j < rc; j++)
        {
            virDomainStatsRecordPtr record = stats[j];
            char tenantuuid[4096] = {0};
            custom::virDomainGetTenant(record->dom, &tenantuuid[0]);

            char uuid[4096] = {0};
            virDomainGetUUIDString(record->dom, uuid);

            for (int k = 0; k < record->nparams; k++)
            {
                // input is vcpu, id, param
                std::vector<std::string> fields = custom::split(record->params[k].field);
                if (fields.size() == 4)
                {
                    out.append(custom::format("# TYPE libvirt_%s_%s_%s counter\n", fields[0].c_str(), fields[3].c_str(), fields[2].c_str()));
                    out.append(custom::format("libvirt_%s_%s_%s{domain=\"%s\" blockid=\"%s\", uuid=\"%s\" tenant=\"%s\"} %llu\n",
                                              fields[0].c_str(), fields[3].c_str(), fields[2].c_str(), // => net_bytes_rx
                                              virDomainGetName(record->dom), fields[1].c_str(), 
                                              uuid,
                                              tenantuuid,
                                              record->params[k].value.ul));
                }
            }
        }
    }

}

#endif