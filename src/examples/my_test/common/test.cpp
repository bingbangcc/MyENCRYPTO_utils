/**
 \file 		millionaire_prob.cpp
 \author 	sreeram.sadasivam@cased.de
 \copyright	ABY - A Framework for Efficient Mixed-protocol Secure Two-party Computation
			Copyright (C) 2019 Engineering Cryptographic Protocols Group, TU Darmstadt
			This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published
            by the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.
            ABY is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
            GNU Lesser General Public License for more details.
            You should have received a copy of the GNU Lesser General Public License
            along with this program. If not, see <http://www.gnu.org/licenses/>.
 \brief		Implementation of the millionaire problem using ABY Framework.
 */

#include "test.h"
#include "../../../abycore/circuit/booleancircuits.h"
#include "../../../abycore/sharing/sharing.h"

int32_t test_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
		uint32_t bitlen, uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
	
	// mt_alg表示用什么办法生成乘法三元组，默认是OT
	ABYParty* party = new ABYParty(role, address, port, seclvl, bitlen, nthreads, mt_alg);
	std::vector<Sharing*>& sharings = party->GetSharings();
	Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();
	share *s_alice_money, *s_bob_money, *s_out;
	uint32_t alice_money, bob_money, output;
	
	share* const_val = circ->PutCONSGate(10, bitlen);
	// 测试程序里，输入值由随机数生成，用户不需要输入自己的值
	srand(time(NULL));
	alice_money = rand() % 10;
	bob_money = rand() % 10;

	// if(role == SERVER) {
	// 	// 对于bob来说PutDummyINGate就表示Alice（Server方）的值将来由alice来提供
	// 	s_alice_money = circ->PutDummyINGate(bitlen);
	// 	// bob提供Bob（client方）自己的值
	// 	s_bob_money = circ->PutINGate(bob_money, bitlen, SERVER);
	// } else { 
	// 	//role == CLIENT
	// 	s_alice_money = circ->PutINGate(alice_money, bitlen, CLIENT);
	// 	s_bob_money = circ->PutDummyINGate(bitlen);
	// }

	s_bob_money = circ->PutINGate(bob_money, bitlen, SERVER);
	s_alice_money = circ->PutINGate(alice_money, bitlen, CLIENT);

	std::cout << "Current role is "<< (role ? ALICE : BOB) << "  " << (role ? alice_money : bob_money) << std::endl;
	
	s_out = circ->PutMULGate(s_alice_money, s_bob_money);
	// circ->PutPrintValueGate(s_out, "Mul");
	// s_out = circ->PutADDGate(s_out, const_val);
	// circ->PutPrintValueGate(s_out, "Add");

	s_out = circ->PutOUTGate(s_out, ALL);
	party->ExecCircuit();

	output = s_out->get_clear_value<uint32_t>();
	std::cout << "Testing Problem in " << get_sharing_name(sharing)
				<< " sharing: " << std::endl;
	std::cout << "\nAlice Money:\t" << alice_money;
	std::cout << "\nBob Money:\t" << bob_money;
	std::cout << "\nCircuit Result:\t" << output;
	std::cout << "\nVerify Local Result: \t" << alice_money * bob_money
				<< "\n";
	delete party;
	return 0;
}

share* BuildTestCircuit(share *s_alice, share *s_bob,
		BooleanCircuit *bc) {
	share* out;
	out = bc->PutGTGate(s_alice, s_bob);
	return out;
}
