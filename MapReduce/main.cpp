#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Table.hpp"
#include "MapReduce.hpp"
#include "Selector.hpp"
#include "Projector.hpp"
#include "Joiner.hpp"
#include "SemiJoin.hpp"
#include "FullReducer.hpp"

namespace mpi = boost::mpi;

void optimize_join(std::vector<Table>& input)
{
	mpi::communicator world;
	std::vector<std::vector<std::string>> attrs;
	for (const auto& t : input)
	{
		attrs.push_back(t.attribute());
	}
	FullReducer reducer(attrs);
	auto list = reducer.build();

	/*
	for (auto i : list)
	{
		for (auto j : i.first)
			std::cout << j <<' ';
		std::cout << '|';
		for (auto j : i.second)
			std::cout << j << ' ';
		std::cout << std::endl;
	}
	*/
	for (const auto& p : list)
	{
		//for (auto t : input)
		//	t.print();
		std::vector<int> param1;
		std::vector<int> param2;
		for (int i = 0; i < p.first.size(); ++i)
		{
			for (int j = 0; j < p.second.size(); ++j)
			{
				if (p.first[i] == p.second[j])
				{
					param1.push_back(i);
					param2.push_back(j);
				}
			}
		}
		/*for (int i : param1)
			std::cout << i << ' ';
		std::cout << std::endl;
		for (int i : param2)
			std::cout << i << ' ';
		std::cout << std::endl;*/
		int t1 = 0;
		int t2 = 0;
		for (int i = 0; i < input.size(); ++i)
		{
			if (p.first == input[i].attribute())
				t1 = i;
			if (p.second == input[i].attribute())
				t2 = i;
		}
		//std::cout << t1 << ' ' << t2;
		MapReduce<SemiJoiner> sj(world, SemiJoiner(param1, param2));
		auto&& res = sj.start({ input[t1], input[t2] });
		std::cout << "semi-join " << t1 << std::endl;
		res.print();
		input[t1].update(std::move(res));
	}
	auto attr = input[0].attribute();
	auto table = input[0];
	table.print();
	for (int i = 1; i < input.size(); ++i)
	{
		std::vector<int> param1;
		std::vector<int> param2;
		auto attr2 = input[i].attribute();
		for (int j = 0; j < attr.size(); ++j)
		{
			for (int k = 0; k < attr2.size(); ++k)
			{
				if (attr[j] == attr2[k])
				{
					param1.push_back(j);
					param2.push_back(k);
				}
			}
		}
		MapReduce<Joiner> sj(world, Joiner(param1, param2));
		auto&& res = sj.start({ table, input[i] });
		res.print();
		table.update(res);
		std::cout << "join " << world.rank() << std::endl;
		std::vector<std::string> new_attr;
		for (int j = 0; j < param1.size(); ++j)
			new_attr.push_back(attr[param1[j]]);
		for (int j = 0; j < attr.size(); ++j)
			if (std::find(param1.begin(), param1.end(), j) == param1.end())
				new_attr.push_back(attr[j]);
		for (int j = 0; j < attr2.size(); ++j)
			if (std::find(param2.begin(), param2.end(), j) == param2.end())
				new_attr.push_back(attr2[j]);
		for (auto a : new_attr)
		{
			std::cout << a << ' ';
		}
		std::cout << std::endl;
		attr = new_attr;
	}
	table.print();
}

Table generateTables(std::string address, int numcol, bool sample=true)
{
	std::vector<std::vector<std::string>> table;
	std:: ifstream fp(address);
	std::string line;

	// first line (col names)
	std::getline(fp, line);
	std::vector<std::string> attr;
	std::string item;
	std::istringstream readstr(line);
	for (int j = 0; j < numcol; j++)
	{
		std::getline(readstr, item, ',');
		attr.push_back(item);
	}

	int maxnum = 100;
	if (sample)
	{
		int cnt = 0;
		while (std::getline(fp, line) && cnt < maxnum)
		{
			std::vector<std::string> data_line;
			std::istringstream readstr(line);
			for (int j = 0; j < numcol; j++)
			{
				std::getline(readstr, item, ',');
				data_line.push_back(item);
			}
			table.push_back(data_line);
			cnt++;
		}
	}
	else
	{
		while (std::getline(fp, line))
		{
			std::vector<std::string> data_line;
			std::istringstream readstr(line);
			for (int j = 0; j < numcol; j++)
			{
				std::getline(readstr, item, ',');
				data_line.push_back(item);
			}
			table.push_back(data_line);
		}
	}

	return Table(table, attr);
}

int main()
{
	mpi::environment env;
	mpi::communicator world;

	//Table t;
	//Table t2;
	//Table t3;
	auto pred = [](auto v) { return v[0] == "a"; };
	Selector<decltype(pred)> sel(pred);
	MapReduce<Selector<decltype(pred)>> mr(world, sel);
	//Projector proj({ 0, 1, 3 });


	/*if (world.rank() == 0)
	{
		t = Table(std::vector<std::vector<std::string>>
		{
			std::vector<std::string>{"a", "1", "2", "3"},
				std::vector<std::string>{"a", "1", "2", "3"},
				std::vector<std::string>{"b", "1", "2", "3"},
				std::vector<std::string>{"c", "1", "2", "3"},
				std::vector<std::string>{"b", "1", "2", "3"},
				std::vector<std::string>{"a", "1", "2", "3"},
				std::vector<std::string>{"a", "1", "2", "3"},
				std::vector<std::string>{"d", "1", "2", "3"},
				std::vector<std::string>{"b", "1", "2", "3"},
		}, { "A", "B", "C", "D" });
		t2 = Table(std::vector<std::vector<std::string>>
		{
			std::vector<std::string>{"a", "1"},
				std::vector<std::string>{"b", "1"},
				std::vector<std::string>{"b", "2"},
				std::vector<std::string>{"c", "4"},
		}, { "A", "B" });
		t3 = Table(std::vector<std::vector<std::string>>
		{
			std::vector<std::string>{"1", "A"},
				std::vector<std::string>{"1", "B"},
				std::vector<std::string>{"2", "C"},
				std::vector<std::string>{"3", "C"},
		}, { "B", "C" });

	}*/
	
	auto t = Table(std::vector<std::vector<std::string>>
	{
		std::vector<std::string>{"a", "1", "1"},
			std::vector<std::string>{"a", "2", "1"},
			std::vector<std::string>{"b", "3", "2"},
			std::vector<std::string>{"b", "4", "2"},
			std::vector<std::string>{"c", "5", "3"},
			std::vector<std::string>{"c", "6", "3"},
	}, { "A", "B", "C" });
	auto t2 = Table(std::vector<std::vector<std::string>>
	{
		std::vector<std::string>{"1", "!", "1"},
			std::vector<std::string>{"2", "@", "1"},
			std::vector<std::string>{"3", "#", "3"},
			std::vector<std::string>{"3", "$", "5"},
			std::vector<std::string>{"4", "%", "3"},
			std::vector<std::string>{"5", "^", "3"},
	}, { "C", "D","E" });
	auto t3 = Table(std::vector<std::vector<std::string>>
	{
		std::vector<std::string>{"a", "1", "u"},
			std::vector<std::string>{"a", "1", "i"},
			std::vector<std::string>{"b", "1", "o"},
			std::vector<std::string>{"c", "5", "j"},
			std::vector<std::string>{"c", "3", "k"},
			std::vector<std::string>{"d", "2", "l"},
	}, {"A", "E", "F" });
	auto t4 = Table(std::vector<std::vector<std::string>>
	{
		std::vector<std::string>{"a", "1", "1"},
			std::vector<std::string>{"b", "2", "1"},
			std::vector<std::string>{"b", "1", "2"},
			std::vector<std::string>{"c", "3", "3"},
			std::vector<std::string>{"c", "3", "5"},
			std::vector<std::string>{"d", "2", "3"},
	}, { "A", "C", "E" });
	// std::vector<Table> tables = { t, t2, t3, t4 };

	Table base = generateTables("./data/base.csv", 124, false);
	Table trans = generateTables("./data/trans.csv", 52, false);
	std::vector<Table> tables = { base, trans };
	// table.print();

	optimize_join(tables);

	//mr.start(t);

	//MapReduce<Projector> mr2(world, { 0, 1, 3 });
	//mr2.start(t);

	//MapReduce<Joiner> mr3(world, Joiner({ 1 }, { 0 }));
	//mr3.start({ t2,t3 });
	//MapReduce<SemiJoiner> mr4(world, SemiJoiner({ 1 }, { 0 }));
	//mr4.start({ t2, t3 });
	return 0;
}



