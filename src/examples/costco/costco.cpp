#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>

//Utility libs
#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>

//ABY Party class
#include "../../abycore/aby/abyparty.h"
#include "../../abycore/circuit/arithmeticcircuits.h"
#include "../../abycore/circuit/booleancircuits.h"
#include "../../abycore/sharing/sharing.h"

int32_t read_test_options(int32_t* argcp, char*** argvp, e_role* role,
    std::string *circ_file, uint32_t* bitlen, uint32_t* nvals, uint32_t* nrounds, uint32_t* secparam,
    std::string* address, uint16_t* port, int32_t* test_op, uint32_t* share_type) {

  uint32_t int_role = 0, int_port = 0;

  parsing_ctx options[] =
      { { (void*) &int_role, T_NUM, "r", "Role: 0/1", true, false }, {
          (void*) circ_file, T_STR, "c", "circuit file", true, false }, {
          (void*) nvals, T_NUM, "n",
          "Number of parallel operation elements", false, false }, {
          (void*) nrounds, T_NUM, "i",
          "Number of rounds", false, false }, {
          (void*) bitlen, T_NUM, "b", "Bit-length, default 32", false,
          false }, { (void*) secparam, T_NUM, "s",
          "Symmetric Security Bits, default: 128", false, false }, {
          (void*) address, T_STR, "a",
          "IP-address, default: localhost", false, false }, {
          (void*) &int_port, T_NUM, "p", "Port, default: 7766", false,
          false }, { (void*) test_op, T_NUM, "t",
          "Single test (leave out for all operations), default: off",
          false, false }, {
          (void*) share_type, T_NUM, "m",
          "Circuit protocol 0=ARITH, 1=BOOL, 2=YAO, default: 0", false, false } 
	  };

  if (!parse_options(argcp, argvp, options,
      sizeof(options) / sizeof(parsing_ctx))) {
    print_usage(*argvp[0], options, sizeof(options) / sizeof(parsing_ctx));
    std::cout << "Exiting" << std::endl;
    exit(0);
  }

  assert(int_role < 2);
  *role = (e_role) int_role;

  if (int_port != 0) {
    assert(int_port < 1 << (sizeof(uint16_t) * 8));
    *port = (uint16_t) int_port;
  }

  //delete options;

  return 1;
}

int32_t CircuitGen(const std::string& circ_file_name, e_role role, const std::string& address, uint16_t port, uint32_t nrounds, seclvl seclvl, uint32_t bitlen, uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
	std::vector<std::string> gates;
	std::map<std::string, std::vector<std::string>> inputs;

	std::ifstream circ_file;
	circ_file.open(circ_file_name);
	std::string line;
	std::vector<std::string> tokens;
	while (std::getline(circ_file, line)) {
		if (boost::starts_with(line, "#")) {
			continue;
		}
		boost::split(tokens, line, [](char c){return c == ' ';});
		auto it = tokens.begin();
		std::string curr_node = *it;
		//std::cout << curr_node << " ";
		gates.push_back(curr_node);
		it++;
		for (it; it != tokens.end(); it++){
			inputs[*it].push_back(curr_node);
			//std::cout << *it << " ";
		}
		//std::cout << std::endl;
		tokens.clear();
	}
	circ_file.close();

	ABYParty* party = new ABYParty(role, address, port, seclvl, bitlen, nthreads, mt_alg);
	std::vector<Sharing*>& sharings = party->GetSharings();
	Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();
  ArithmeticCircuit* arith_circ = (ArithmeticCircuit*) sharings[S_ARITH]->GetCircuitBuildRoutine();
  BooleanCircuit* yao_circ = (BooleanCircuit*) sharings[S_YAO]->GetCircuitBuildRoutine();
  BooleanCircuit* bool_circ = (BooleanCircuit*) sharings[S_BOOL]->GetCircuitBuildRoutine();
	std::map<std::string, share*> output_shares;
	std::vector<share*> outputs;
	for (uint32_t r = 0; r < nrounds; r++) {
		srand(12345);
		for (std::string g : gates) {
			std::string g_type = g.substr(0, g.find("_"));
			std::vector<std::string> input_gates = inputs[g];
			if (g_type == "INPUT0") {
				//uint32_t val = rand();
				uint32_t val = 1;
				if(role == SERVER) {
					output_shares[g] = circ->PutINGate(val, bitlen, SERVER);
				} else {
					output_shares[g] = circ->PutDummyINGate(bitlen);
				}
			} else if (g_type == "INPUT1") {
				//uint32_t val = rand();
				uint32_t val = 1;
				if(role == SERVER) {
					output_shares[g] = circ->PutDummyINGate(bitlen);
				} else {
					output_shares[g] = circ->PutINGate(val, bitlen, CLIENT);
				}
      } else if (g_type == "A2Y") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = yao_circ->PutA2YGate(input);
      } else if (g_type == "A2B") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = circ->PutA2BGate(input, yao_circ);
      } else if (g_type == "B2Y") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = yao_circ->PutB2YGate(input);
      } else if (g_type == "B2A") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = arith_circ->PutB2AGate(input);
      } else if (g_type == "Y2B") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = bool_circ->PutY2BGate(input);
      } else if (g_type == "Y2A") {
	share *input = output_shares[input_gates[0]];
        output_shares[g] = circ->PutY2AGate(input, yao_circ);
			} else if (g_type == "OUTPUT") {
				share *input = output_shares[input_gates[0]];
				std::string input_t = input_gates[0].substr(0, input_gates[0].find("_"));
				if (input_t == "A2Y" || input_t == "B2Y") {
					output_shares[g] = yao_circ->PutOUTGate(input, ALL);
				} else if (input_t == "A2B" || input_t == "Y2B") {
					output_shares[g] = bool_circ->PutOUTGate(input, ALL);
				} else if (input_t == "B2A" || input_t == "Y2A") {
					output_shares[g] = arith_circ->PutOUTGate(input, ALL);
				} else {
					output_shares[g] = circ->PutOUTGate(input, ALL);
				}
				outputs.push_back(output_shares[g]);
			} else {
				share* input1 = output_shares[input_gates[0]];
				share* input2 = output_shares[input_gates[1]];
				if (g_type == "MUL") {
					output_shares[g] = circ->PutMULGate(input1, input2);
				} else if (g_type == "ADD") {
					output_shares[g] = circ->PutADDGate(input1, input2);
				} else if (g_type == "XOR") {
					output_shares[g] = circ->PutXORGate(input1, input2);
				} else if (g_type == "AND") {
					output_shares[g] = circ->PutANDGate(input1, input2);
				}
			}
		}

		party->ExecCircuit();
		uint32_t output;
		/*
		for (share *o: outputs) {
			output = o->get_clear_value<uint32_t>();
			std::cout << output << " ";
		}
		*/
		std::cout << "\t";
		std::cout << party->GetTiming(P_SETUP) << "\t"
			<< party->GetTiming(P_ONLINE) << "\t"
			<< party->GetSentData(P_SETUP)+party->GetReceivedData(P_SETUP) << "\t"
			<< party->GetSentData(P_ONLINE)+party->GetReceivedData(P_ONLINE) << "\t"
			<< sharings[sharing]->GetNumNonLinearOperations() << "\t"
			<< sharings[sharing]->GetMaxCommunicationRounds() << std::endl;
			
		party->Reset();
		output_shares.clear();
		outputs.clear();
	}
	delete party;
	return 0;
}

int main(int argc, char** argv) {
  e_role role;
  uint32_t bitlen = 32, nvals = 31, nrounds = 10, secparam = 128, nthreads = 1;
  uint16_t port = 7766;
  std::string address = "127.0.0.1", circ_file;
  int32_t test_op = -1;
  e_mt_gen_alg mt_alg = MT_OT;
  uint32_t share_type = 0;

  read_test_options(&argc, &argv, &role, &circ_file, &bitlen, &nvals, &nrounds, &secparam,
      &address, &port, &test_op, &share_type);


  seclvl seclvl = get_sec_lvl(secparam);
  e_sharing sharing = S_ARITH;
  if (share_type == 1) {
	  sharing = S_BOOL;
  } else if (share_type == 2) {
	  sharing = S_YAO;
  }
  std::cout << "Sharing: " << share_type << std::endl;

  //evaluate the millionaires circuit using Yao
  // test_circuit(role, address, port, seclvl, 32, nvals, ndepth, nrounds, nthreads, mt_alg, S_ARITH);
  CircuitGen(circ_file, role, address, port, nrounds, seclvl, 32, nthreads, mt_alg, sharing);
  //evaluate the millionaires circuit using GMW
  //test_millionaire_prob_circuit(role, address, port, seclvl, 32,
  //    nthreads, mt_alg, S_BOOL);

  return 0;
}

