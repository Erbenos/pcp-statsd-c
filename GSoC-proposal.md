## Contact information
- Name - Miroslav Foltýn
- Phone - +420 732 204 141
- Email - email@miroslavfoltyn.com
- Location - Czech Republic
- Website - miroslavfoltyn.com
- Postal Address: 
    - Československé armády 338
    - Budišov nad Budišovkou 74787
    - Czech Republic
    
I also hangout on "The Programmer's hangout" Discord channel. Nick is @Erbenos#1658.

### Contact information in case of an emergency
- Name - Kateřina Biolková
- Phone - +420 739 981 205
- Email - biolkova.katerina@gmail.com
- Name - Jana Foltýnová
- Phone - +420 777 106 131
- Email - foltynova@linaset.cz

# PCP PMDA agent for StatsD in C

## Bibligraphy and Past Experience
I am Master's Computer Science student at Palacky University in Olomouc.

I have been working as a freelance web developer for 3 years, while studying simultaneously. Last 6 months or so I am part-time working at Computer Center of Palacky University, where me and my colleagues build upon and improve existing university's internal applications for employees and students alike. I mostly do front-end stuff now, altrought I am able to do back-end if need be (in either C# or PHP, I have some NodeJS experience, but only for client side app's - with stuff like Electron - not with actually coding server-side logic).
I have little low-level programming experience, only did some microcontroller programming in Assembly, C and VHDL in my secondary school years. There were some C and Assembly tasks while studying at University as well. I am looking forward to gaining more experience in C and meeting people from the community.

If my proposal get's accepted, I will fully commit myself to GSoC the moment coding begins.

### Website for Blue Partners s.r.o.
I successfully completed both front-end and back-end of given website design in collaboration with marketer (Lukáš Hladeček), copywriter (Karel Melecký), designer (Michal Maleňák) and SEO analyst (Šárka Jakubcová). The project was completed in tight deadline, as the marketting campaign was already setup and delays would be costly. Site was further improved months down the line with more functionality as requested by the client. Most important part was interactive test that aimed at inspecting company owners perception of their company's technical security and presenting gathered data in clear and informative way.

[Check it out](https://www.bluepartners.cz/en/)

### Redesign of Palacky University in Olomouc Helpdesk
In collaboration with Computer Center of Palacky University I implemented redesign of university's Helpdesk application that serves as a main place where both students and employees can submit tickets regarding various issues they may have with either their studies or jobs. I communicated closely with day-to-day users of Helpdesk about their feedback to WIP builds to accustom application's UI to their everyday needs and make switching from the older design smoother, and mainly more pleasant, experience.

[Check it out](https://helpdesk.upol.cz/)

## Abstract
*StatsD* is simple, text-based UDP protocop for receiving monitoring data of applications in architecture client-server. As of right now, there is no StatsD implementation for PCP available, other then [this](https://github.com/lzap/pcp-mmvstatsd) which is not suitable for production environment.

Goal of this project is to write PMDA agent for PCP in C, that would receive StatsD UDP packets and then aggregate and transfer handled data to PCP. There would be 3 basic types of metrics: counter, duration and gauge. Agent is to be build with modular architecture in mind with an option of changing implementation of both aggregator and parsers, which will allow to accurately describe differences between approaches to aggregation and text protocol parsing. Since the PMDA API is based on around callbacks the design has to be multithreaded.

Agent is to be fully configurable with either PCP configuration options and/or separate configuration file. Writing  integration tests is also in the scope of the project.

## Proposal Timeline

### Community Bonding Period
- To familiarize myself with PCP, with the help of my mentor
- To study and gain an understanding of how to write a PMDA for PCP, with help of [PCP Programmer's Guide](https://pcp.io/books/PCP_PG/pdf/pcp-programmers-guide.pdf)
- Clear up issues regarding project development setup and/or project timeline and it's milestones
- To show what is already done (as the project is also my Master's thesis that I started to work on since January 2019) 

### May 27 - June 28
- To incorporate HDR histogram for data aggregation
- To incorporate Ragel parser
- To write end-to-end/integration tests.

### June 29 - July 26
- To connect what's done with PCP
- To write end-to-end/integration tests

### July 27 - August 19
- To do a performance analysis and optimize program based on results
- To write documentation
- Bounce period for any delays