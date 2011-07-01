namespace music { namespace braille {

enum value_category
{
  large, small
};

class value_proxy
{
  ambiguous::value value_type;
  value_category category;
  unsigned dots;
  rational* final_type;
public:
  value_proxy(ambiguous::note& note, value_category const& category)
  : value_type(note.ambiguous_value), category(category), dots(note.dots)
  , final_type(&note.type)
  {
    BOOST_ASSERT(*final_type == music::rational(0));
  }

  value_proxy(ambiguous::rest& rest, value_category const& category)
  : value_type(rest.ambiguous_value), category(category), dots(rest.dots)
  , final_type(&rest.type)
  {
    BOOST_ASSERT(*final_type == music::rational(0));
  }

  value_proxy(ambiguous::chord& chord, value_category const& category)
  : value_type(chord.base.ambiguous_value), category(category), dots(chord.base.dots)
  , final_type(&chord.base.type)
  {
    BOOST_ASSERT(*final_type == music::rational(0));
  }

  rational as_rational() const {
    rational base;
    switch (value_type) {
    case ambiguous::whole_or_16th:
      base = rational(1, category==large? 1: 16);
      break;
    case ambiguous::half_or_32th:
      base = rational(1, category==large? 2: 32);
      break;
    case ambiguous::quarter_or_64th:
      base = rational(1, category==large? 4: 64);
      break;
    case ambiguous::eighth_or_128th:
      base = rational(1, category==large? 8: 128);
      break;
    default:
      std::cout << "value type: " << value_type << std::endl;
      BOOST_ASSERT(false);
    }
    return base;
  }
};

class value_proxy_list : public std::vector< std::vector<value_proxy> >,
                         public boost::static_visitor<void>
{
  enum choices { just_large, just_small, both };
  choices choice;
  ambiguous::value last_value;
public:
  value_proxy_list(ambiguous::partial_measure_voice& voice)
  : std::vector< std::vector<value_proxy> >()
  , choice(both), last_value(ambiguous::unknown)
  {
    std::for_each(voice.begin(), voice.end(), boost::apply_visitor(*this));
  }
  template<class Note>
  void operator()(Note& note) {
    value_type possibilities;
    if (choice == just_large) {
      if (last_value == ambiguous::unknown) last_value = value(note);
      if (last_value == value(note)) {
        possibilities.push_back(value_proxy(note, large));
      } else {
        last_value = ambiguous::unknown;
        choice = both;
      }
    } else if (choice == just_small) {
      if (last_value == ambiguous::unknown) last_value = value(note);
      if (last_value == value(note)) {
        possibilities.push_back(value_proxy(note, small));
      } else {
        last_value = ambiguous::unknown;
        choice = both;
      }
    }
    if (choice == both) {
      possibilities.push_back(value_proxy(note, large));
      possibilities.push_back(value_proxy(note, small));
    }
    BOOST_ASSERT(possibilities.size() <= 2);
    push_back(possibilities);
  }
  void operator()(ambiguous::value_distinction value_distinction) {
    switch (value_distinction) {
    case ambiguous::large_follows:
      choice = just_large;
      break;
    case ambiguous::small_follows:
      choice = just_small;
      break;
    default: BOOST_ASSERT(false);
    }
  }
  void operator()(ambiguous::simile&) {
  }
private:
  template<class NoteOrRest>
  static ambiguous::value value(NoteOrRest& note) {
    return note.ambiguous_value;
  }
  static ambiguous::value value(ambiguous::chord& chord) {
    return value(chord.base);
  }
};

std::vector< std::vector<value_proxy> >
disambiguate( value_proxy_list::const_iterator const& first
            , value_proxy_list::const_iterator const& last
            , rational const& max_length
            )
{
  std::vector< std::vector<value_proxy> > result;
  if (first == last) return result;
  for (std::vector<value_proxy>::const_iterator
       iter = first->begin(); iter != first->end(); ++iter) {
    if (iter->as_rational() <= max_length) {
      if (first+1 == last) {
        std::vector<value_proxy> tail;
        tail.push_back(*iter);
        result.push_back(tail);
      } else {
        std::vector< std::vector<value_proxy> >
        tail = disambiguate(first + 1, last, max_length - iter->as_rational());
        for (std::vector< std::vector<value_proxy> >::iterator
             tail_iter = tail.begin(); tail_iter != tail.end(); ++tail_iter) {
          tail_iter->insert(tail_iter->begin(), *iter);
          result.push_back(*tail_iter);
        }
      }
    }
  }
  return result;
}

rational
duration( std::vector<value_proxy> const& proxies )
{
  rational value;
  for (std::vector<value_proxy>::const_iterator
       iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    value += iter->as_rational();
  }
  return value;
}

class proxied_partial_measure
: public std::vector< std::vector< std::vector<value_proxy> > >
{
  static std::vector< std::vector< std::vector<value_proxy> > >
  recurse( std::vector< std::vector< std::vector<value_proxy> > >::iterator const& first
         , std::vector< std::vector< std::vector<value_proxy> > >::iterator const& last
         , std::vector< std::vector<value_proxy> > voice_stack
         , rational const& length
         )
  {
    std::vector< std::vector< std::vector<value_proxy> > > result;
    if (first == last) {
      result.push_back(voice_stack);
    } else {
      for (std::vector< std::vector<value_proxy> >::iterator
           iter = first->begin(); iter != first->end(); ++iter)
      {
        if (duration(*iter) == length) {
          std::vector< std::vector< std::vector<value_proxy> > > tmp;
          std::vector< std::vector<value_proxy> > vs(voice_stack);
          vs.push_back(*iter);
          tmp = recurse(first + 1, last, vs, length);
          result.insert(result.end(), tmp.begin(), tmp.end());
        }
      }
    }
    return result;
  }
public:
  proxied_partial_measure( ambiguous::partial_measure& partial_measure
                         , rational const& max_length
                         )
  : std::vector< std::vector< std::vector<value_proxy> > >()
  {
    std::vector< std::vector< std::vector<value_proxy> > > voices;
    for (ambiguous::partial_measure::iterator
         partial_voice = partial_measure.begin();
         partial_voice != partial_measure.end(); ++partial_voice)
    {
      value_proxy_list candidates(*partial_voice);
      voices.push_back(disambiguate(candidates.begin(), candidates.end(),
                                    max_length));
    }
    if (!voices.empty()) {
      for (std::vector< std::vector<value_proxy> >::const_iterator
           notes = voices.begin()->begin(); notes != voices.begin()->end();
           ++notes)
      {
        std::vector< std::vector<value_proxy> > voice_stack;
        voice_stack.push_back(*notes);
        std::vector< std::vector< std::vector<value_proxy> > >
        interpretations = recurse(voices.begin() + 1, voices.end(),
                                  voice_stack, duration(*notes));
        insert(end(), interpretations.begin(), interpretations.end());
      }
    }
  }
};

rational
duration( std::vector< std::vector<value_proxy> > const& proxies )
{
  rational value;
  for (std::vector<value_proxy>::const_iterator
       iter = proxies.begin()->begin(); iter != proxies.begin()->end(); ++iter)
  {
    value += iter->as_rational();
  }
  return value;
}

class proxied_voice
: public std::vector< std::vector< std::vector< std::vector<value_proxy> > > >
{
  static std::vector< std::vector< std::vector< std::vector<value_proxy> > > >
  recurse( ambiguous::voice::iterator const& first
         , ambiguous::voice::iterator const& last
         , std::vector< std::vector< std::vector<value_proxy> > > part_stack
         , rational const& max_length
         )
  {
    BOOST_ASSERT(max_length >= 0);
    std::vector< std::vector< std::vector< std::vector<value_proxy> > > > result;
    if (first == last) {
      result.push_back(part_stack);
    } else {
      proxied_partial_measure ppm(*first, max_length);
      for (proxied_partial_measure::iterator
           possibility = ppm.begin(); possibility != ppm.end(); ++possibility)
      {
        std::vector< std::vector< std::vector< std::vector<value_proxy> > > > tmp;
        std::vector< std::vector< std::vector<value_proxy> > > ps(part_stack);
        ps.push_back(*possibility);
        tmp = recurse(first + 1, last, ps, max_length - duration(*possibility));
        result.insert(result.end(), tmp.begin(), tmp.end());
      }
    }
    return result;
  }
public:
  proxied_voice(ambiguous::voice voice, rational max_length)
  : std::vector< std::vector< std::vector< std::vector<value_proxy> > > >()
  {
    if (!voice.empty()) {
      proxied_partial_measure ppm(*voice.begin(), max_length);
      for (proxied_partial_measure::iterator
           possibility = ppm.begin(); possibility != ppm.end(); ++possibility)
      {
        std::vector< std::vector< std::vector<value_proxy> > > part_stack;
        part_stack.push_back(*possibility);
        std::vector< std::vector< std::vector< std::vector<value_proxy> > > >
        interpretations = recurse(voice.begin() + 1, voice.end(),
                                  part_stack, max_length - duration(*possibility));
        insert(end(), interpretations.begin(), interpretations.end());
      }
    }
  }
};

rational
duration( std::vector< std::vector< std::vector<value_proxy> > > const& proxies )
{
  rational value;
  for (std::vector< std::vector< std::vector<value_proxy> > >::const_iterator
       iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    value += duration(*iter);
  }
  return value;
}

class proxied_measure
: public std::vector< std::vector< std::vector< std::vector< std::vector<value_proxy> > > > >
{
  static std::vector<value_type>
  recurse( ambiguous::measure::iterator const& first
         , ambiguous::measure::iterator const& last
         , value_type voice_stack
         , rational const& max_length
         , rational const& real_length
         )
  {
    BOOST_ASSERT(max_length >= 0);
    std::vector< value_type > result;
    if (first == last) {
      result.push_back(voice_stack);
    } else {
      proxied_voice pv(*first, max_length);
      for (proxied_voice::iterator
           possibility = pv.begin(); possibility != pv.end(); ++possibility)
      {
	if (real_length == duration(*possibility)) {
	  value_type vs(voice_stack);
	  vs.push_back(*possibility);
	  std::vector< value_type >
	  interpretations = recurse(first + 1, last, vs, max_length, real_length);
	  result.insert(result.end(), interpretations.begin(), interpretations.end());
	}
      }
    }
    return result;
  }
public:
  proxied_measure(ambiguous::measure measure, rational max_length)
  : std::vector< std::vector< std::vector< std::vector< std::vector<value_proxy> > > > >()
  {
    if (!measure.empty()) {
      proxied_voice pv(*measure.begin(), max_length);
      for (proxied_voice::iterator
           possibility = pv.begin(); possibility != pv.end(); ++possibility)
      {
	value_type voice_stack;
        voice_stack.push_back(*possibility);
	std::vector< value_type >
        interpretations = recurse(measure.begin() + 1, measure.end(),
                                  voice_stack, max_length, duration(*possibility));
        insert(end(), interpretations.begin(), interpretations.end());
      }
    }
  }
};

}}
