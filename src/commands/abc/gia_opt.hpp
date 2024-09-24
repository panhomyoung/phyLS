/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file gia_opt.hpp
 *
 * @brief  Optimize the current GIA using ABC '&*' commands
 *
 * @author Jiaxiang Pan
 * @since  2024/09/20
 */

#ifndef GIA_OPT_HPP
#define GIA_OPT_HPP

#include <iostream>
#include <string>

#include <mockturtle/networks/aig.hpp>
#include "../core/abc_gia.hpp"
#include "../core/abc.hpp"

using namespace std;
using namespace mockturtle;

namespace alice
{
    class gia_opt_command : public command
    {
    public:
        explicit gia_opt_command(const environment::ptr &env)
            : command(env, "Optimize the current GIA using ABC '&*' commands")
        {
            add_option("-s, --string", script, "set the opt string in ABC9 [default = &ps]");
        }

    protected:
        void execute()
        {
            clock_t begin, end;
            double totalTime;

            begin = clock();

            if (store<pabc::Gia_Man_t *>().size() == 0)
                std::cerr << "Empty GIA\n";
            else
            {
                pabc::Gia_Man_t *gia_ntk = store<pabc::Gia_Man_t *>().current();
                pabc::Gia_Man_t * gia = pabc::Gia_ManDup( gia_ntk );
                pabc::Abc_Frame_t * abc = pabc::Abc_FrameGetGlobalFrame();
                pabc::Abc_FrameUpdateGia(abc, gia);
                const int success = pabc::Cmd_CommandExecute(abc, script.c_str());
                if (success != 0) {
                printf("syntax error in script\n");
                }
                pabc::Gia_Man_t * new_gia = pabc::Abc_FrameGetGia(abc);
                fmt::print(" After Run ABC9 command: {} [GIA] PI/PO = {}/{}  nodes = {}  level = {}\n ",
                    script, 
                    pabc::Gia_ManPiNum(new_gia), 
                    pabc::Gia_ManPoNum(new_gia), 
                    Gia_ManAndNum(new_gia), 
                    pabc::Gia_ManLevelNum(new_gia));

                store<pabc::Gia_Man_t *>().extend();
                store<pabc::Gia_Man_t *>().current() = new_gia;
            }

            end = clock();
            totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

            cout.setf(ios::fixed);
            cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
            
        }

    private:
        std::string script = "&ps"; // default &ps
    };

    ALICE_ADD_COMMAND(gia_opt, "ABC")

} // namespace alice

#endif
